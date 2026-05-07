// shadertype=hlsl
// Inline-snippet (NOT a standalone header — see PTRaygenIntegrator.hlsl
// "snippet split" comment). Spliced into RunPathIntegrator's bounce loop
// after the NEE block and before the BSDF importance-sample. Verbatim
// extraction from the pre-split inline form; no semantic change.
// In-scope dependencies: payload, ray, bounce, throughput, radiance,
// vertexDirectLighting, prev_cell_index, prev_throughput, plus globals
// g_Frame / g_FrameCount / g_HashGridCacheConstants /
// g_HashGridCache_HashBuffer / g_HashGridCache_DecayTileBuffer /
// g_HashGridCache_UpdateCellValueBuffer /
// g_HashGridCache_UpdateCellValueIndirectBuffer /
// g_HashGridCache_ValueBuffer / g_HashGridCache_ValueIndirectBuffer.
// PT_DENOISE_ENABLED nested branch reads isSpecularPath /
// radianceDiffuse / radianceSpecular. Loop-control mutation
// (cacheTerminated → break) is preserved verbatim — the snippet
// declares the local and uses it in the same scope as the caller's
// loop, so the break exits the bounce loop.

#if PT_HASH_GRID_CACHE_ENABLED
        // Site-3 read + secondary-vertex writes at every secondary+ vertex.
        // Bounce 0 is never cached — primary visibility is always re-traced
        // fresh (audit D1: cache acts as a terminator only at indirect
        // hits). Capsaicin's glossy-reflections pattern adapted to a
        // loop-per-bounce raygen (gi1.comp:2865-2900).
        //
        // Two writes happen at this site:
        //   (a) THIS-vertex direct light (vertexDirectLighting) into the
        //       direct-lobe scratch UpdateCellValueBuffer at this cell —
        //       Capsaicin PopulateCells equivalent (gi1.comp:2087-2095).
        //   (b) PREVIOUS-vertex secondary-bounce contribution: when this
        //       cell carries a usable mean, atomic-add
        //       `bsdf_over_pdf_at_(N-1) * (directMean + indirectMean)`
        //       into the previous vertex's *indirect* scratch
        //       (UpdateCellValueIndirectBuffer) — Capsaicin
        //       UpdateMultibounceCells (gi1.comp:1948-1989).
        //
        // The read combines the direct and indirect lobes independently
        // — each normalised by its own .w running-mean sample count, then
        // summed before the cache substitution. Mirrors Capsaicin
        // gi1.comp:2891-2896 (TraceReflectionsHandleHit) and the identical
        // sum-of-means form at gi1.comp:2377-2383 (ResolveCells). The two
        // lobes carry their own sample counts via independent UpdateTiles
        // running-mean blocks (gi1.comp:2160-2180 direct + gi1.comp:
        // 2183-2223 indirect). Substitution gate is "either lobe has
        // samples"; per-lobe ternaries protect against 0/0.
        bool cacheTerminated = false;
        if (bounce >= 1u)
        {
            PTHashGridCache_Data data;
            data.eye_position = g_Frame.camera_posWS.xyz;
            data.hit_position = payload.hitPos;
            data.direction    = ray.Direction;
            data.hit_distance = length(payload.hitPos - ray.Origin);

            uint  tile_index;
            bool  is_new_tile;
            uint  cell_index = PTHashGridCache_InsertCell(g_HashGridCacheConstants, data,
                                                         g_HashGridCache_HashBuffer,
                                                         tile_index, is_new_tile);

            if (cell_index != kPTHashGridCache_InvalidId)
            {
                // Bump tile-decay timestamp so PTHashGridCachePurgeTilesPass
                // keeps the tile alive while it is being touched.
                uint prev_decay;
                InterlockedExchange(g_HashGridCache_DecayTileBuffer[tile_index], g_FrameCount, prev_decay);

                // (a) THIS-vertex direct light (Capsaicin PopulateCells write).
                uint4 quantizedDirect = PTHashGridCache_QuantizeRadiance(vertexDirectLighting);
                uint  prev_atomic;
                InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * cell_index + 0u], quantizedDirect.x, prev_atomic);
                InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * cell_index + 1u], quantizedDirect.y, prev_atomic);
                InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * cell_index + 2u], quantizedDirect.z, prev_atomic);
                InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * cell_index + 3u], quantizedDirect.w, prev_atomic);

                // Read the resolved running means from BOTH lobes. Each is
                // populated by its own PTHashGridCacheUpdateTiles running-mean
                // block earlier this frame (direct: gi1.comp:2160-2180;
                // indirect: gi1.comp:2183-2223). Storage convention is
                // identical (.rgb stores radiance × sample_count, .w stores
                // sample_count — Capsaicin gi1.comp:2165) so each lobe's
                // per-sample mean is .rgb / .w. The two .w channels are
                // independent — direct caps at
                // g_HashGridCacheConstants.max_sample_count, indirect at
                // MAX_MULTIBOUNCE_SAMPLE_COUNT == 16 (Capsaicin gi1.h:63 vs
                // gi1.h:65) — so they cannot share a normaliser. First
                // frame after a cache clear sees both .w == 0 and the outer
                // gate falls through to BSDF sampling.
                float4 directRadiance   = PTHashGridCache_UnpackRadiance(g_HashGridCache_ValueBuffer[cell_index]);
                float4 indirectRadiance = PTHashGridCache_UnpackRadiance(g_HashGridCache_ValueIndirectBuffer[cell_index]);
                if (directRadiance.w > 0.0f || indirectRadiance.w > 0.0f)
                {
                    // Sum-of-means: each lobe contributes its own per-sample
                    // mean independently; the cache substitution multiplies
                    // the combined mean by throughput. Mirrors Capsaicin
                    // gi1.comp:2891-2896 (TraceReflectionsHandleHit) and
                    // gi1.comp:2377-2383 (ResolveCells) — both Capsaicin
                    // read sites use the same lobe-additive form. A lobe
                    // with .w == 0 contributes zero (its accumulator hasn't
                    // received samples yet).
                    float3 directMean   = directRadiance.w   > 0.0f ? directRadiance.rgb   / directRadiance.w   : float3(0.0f, 0.0f, 0.0f);
                    float3 indirectMean = indirectRadiance.w > 0.0f ? indirectRadiance.rgb / indirectRadiance.w : float3(0.0f, 0.0f, 0.0f);
                    float3 mean = directMean + indirectMean;

                    // (b) PREVIOUS-vertex secondary-bounce contribution
                    // (Capsaicin UpdateMultibounceCells, gi1.comp:1948-1989).
                    // Atomic-adds the BRDF/pdf-modulated combined mean into
                    // the previous vertex's *indirect* scratch slot (the
                    // canonical Capsaicin target — direct lobe carries
                    // populate-cells writes only).
                    //
                    // Skipped at bounce==1 (no prior secondary vertex) and
                    // when the prior iteration could not claim a cell.
                    // brdf_over_pdf reduces to throughput/prev_throughput
                    // because both factors share the path-prefix chain.
                    if (prev_cell_index != kPTHashGridCache_InvalidId)
                    {
                        // Component-wise safe divide: when a channel of
                        // prev_throughput collapsed to zero (e.g. albedo == 0
                        // on a surface), keep the contribution at zero rather
                        // than synthesising radiance out of a divide-by-zero.
                        float3 brdf_over_pdf = float3(
                            prev_throughput.x > 0.0f ? throughput.x / prev_throughput.x : 0.0f,
                            prev_throughput.y > 0.0f ? throughput.y / prev_throughput.y : 0.0f,
                            prev_throughput.z > 0.0f ? throughput.z / prev_throughput.z : 0.0f);
                        float3 secondaryContribution = brdf_over_pdf * mean;
                        uint4  quantizedSecondary    = PTHashGridCache_QuantizeRadiance(secondaryContribution);
                        InterlockedAdd(g_HashGridCache_UpdateCellValueIndirectBuffer[4u * prev_cell_index + 0u], quantizedSecondary.x, prev_atomic);
                        InterlockedAdd(g_HashGridCache_UpdateCellValueIndirectBuffer[4u * prev_cell_index + 1u], quantizedSecondary.y, prev_atomic);
                        InterlockedAdd(g_HashGridCache_UpdateCellValueIndirectBuffer[4u * prev_cell_index + 2u], quantizedSecondary.z, prev_atomic);
                        InterlockedAdd(g_HashGridCache_UpdateCellValueIndirectBuffer[4u * prev_cell_index + 3u], quantizedSecondary.w, prev_atomic);
                    }

                    radiance += throughput * mean;
#if PT_DENOISE_ENABLED
                    // Cache-terminated path inherits the path's lobe tag.
                    // bounce >= 1 here (the cache gate), so isSpecularPath
                    // reflects what the primary-hit lobe sample chose.
                    float3 cacheContribution = throughput * mean;
                    if (isSpecularPath)
                        radianceSpecular += cacheContribution;
                    else
                        radianceDiffuse  += cacheContribution;
#endif
                    cacheTerminated = true;
                }

                // Carry forward this iteration's cell + throughput so the
                // NEXT iteration can attribute its cache-mean lookup back
                // here as a secondary-bounce contribution. Saved BEFORE the
                // BSDF importance sample updates throughput, so the next
                // iteration's `throughput / prev_throughput` recovers the
                // BRDF/pdf factor applied between vertices.
                prev_cell_index = cell_index;
                prev_throughput = throughput;
            }
        }

        if (cacheTerminated)
            break;
#endif
