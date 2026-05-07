// shadertype=hlsl
// Inline-snippet (NOT a standalone header — see PTRaygenIntegrator.hlsl
// "snippet split" comment). Spliced into RunPathIntegrator's bounce loop
// after the GBuffer-write site and before the cache Site-3 block.
// Verbatim extraction; no semantic change. In-scope dependencies:
// payload, N, V, albedo, metalness, roughness, bounce, throughput,
// radiance, rng, plus globals g_Frame / g_PointLightCount /
// g_PointLights / g_SphereLightCount / g_SphereLights / SceneAS.
// Toggle-gated lobe-bucket additions reach radianceDiffuse /
// radianceSpecular / isSpecularPath; cache-mode reaches
// vertexDirectLighting.

        // Direct sun lighting with soft shadow (jittered sun disk)
        float3 lightDir = SampleSunDirection(normalize(g_Frame.sun_direction.xyz), Rand2(rng));
        float3 lightIlluminance = g_Frame.sun_illuminance.xyz;

        ShadowPayload shadow;
        shadow.isShadowed = true;
        RayDesc shadowRay;
        shadowRay.Origin    = payload.hitPos + N * RAY_EPSILON;
        shadowRay.Direction = lightDir;
        shadowRay.TMin      = RAY_EPSILON;
        shadowRay.TMax      = RAY_MAX_DISTANCE;
        TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                 0xFF, 0, 0, 1, shadowRay, shadow);

        if (!shadow.isShadowed)
        {
            radiance += throughput * CookTorranceGGX(N, V, lightDir, albedo, metalness, roughness) * lightIlluminance;
#if PT_HASH_GRID_CACHE_ENABLED
            vertexDirectLighting += CookTorranceGGX(N, V, lightDir, albedo, metalness, roughness) * lightIlluminance;
#endif
#if PT_DENOISE_ENABLED
            // NEE at the primary hit is always diffuse (specular-NEE is
            // CL-5). Indirect-bounce NEE follows the path's lobe tag.
            float3 sunNEE = throughput * CookTorranceGGX(N, V, lightDir, albedo, metalness, roughness) * lightIlluminance;
            if (bounce == 0u || !isSpecularPath)
                radianceDiffuse += sunNEE;
            else
                radianceSpecular += sunNEE;
#endif
        }

        // Sky NEE. Visibility-gated environment sampling: cosine-weighted
        // hemisphere sample, shadow ray, and if the sample escapes scene
        // geometry the sky's HDR radiance contributes. Replaces the old
        // "sky on any indirect miss" path — occluded rays (Sponza's roof
        // etc.) contribute zero; visible sky (open atrium, skybox scenes)
        // still lights the surface correctly. No MIS with BSDF sampling
        // because indirect miss no longer carries sky radiance (see miss
        // branch above).
        {
            float2 xiSky = Rand2(rng);
            float3 skyL = UniformSampleHemisphere(xiSky, N);
            float  NdotSky = max(dot(N, skyL), 0.0f);
            if (NdotSky > 0.0f)
            {
                ShadowPayload skyShadow;
                skyShadow.isShadowed = true;
                RayDesc skyRay;
                skyRay.Origin    = payload.hitPos + N * RAY_EPSILON;
                skyRay.Direction = skyL;
                skyRay.TMin      = RAY_EPSILON;
                skyRay.TMax      = RAY_MAX_DISTANCE;
                TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                         0xFF, 0, 0, 1, skyRay, skyShadow);
                if (!skyShadow.isShadowed)
                {
                    // Uniform-hemisphere pdf = 1/(2π). CookTorranceGGX
                    // already returns BRDF·cos, so the estimator
                    //   L·(BRDF·cos)/pdf = L·CookTorrance·2π
                    // no cos-division hazard at grazing angles.
                    float3 skyRadiance = SkyColor(skyL);
                    radiance += throughput * CookTorranceGGX(N, V, skyL, albedo, metalness, roughness) * skyRadiance * TWO_PI;
#if PT_HASH_GRID_CACHE_ENABLED
                    vertexDirectLighting += CookTorranceGGX(N, V, skyL, albedo, metalness, roughness) * skyRadiance * TWO_PI;
#endif
#if PT_DENOISE_ENABLED
                    float3 skyNEE = throughput * CookTorranceGGX(N, V, skyL, albedo, metalness, roughness) * skyRadiance * TWO_PI;
                    if (bounce == 0u || !isSpecularPath)
                        radianceDiffuse += skyNEE;
                    else
                        radianceSpecular += skyNEE;
#endif
                }
            }
        }

        // Point light NEE
        for (uint ptIdx = 0; ptIdx < g_PointLightCount; ptIdx++)
        {
            float3 ptPos     = g_PointLights[ptIdx].position.xyz;
            float  ptRadius  = g_PointLights[ptIdx].luminousFlux.w;
            float3 ptFlux    = g_PointLights[ptIdx].luminousFlux.xyz;

            float3 toLight = ptPos - payload.hitPos;
            float  dist    = length(toLight);
            if (dist > ptRadius)
                continue;

            float3 L    = toLight / dist;
            float NdotL = max(dot(N, L), 0.0f);
            if (NdotL <= 0.0f)
                continue;

            ShadowPayload ptShadow;
            ptShadow.isShadowed = true;
            RayDesc ptShadowRay;
            ptShadowRay.Origin    = payload.hitPos + N * RAY_EPSILON;
            ptShadowRay.Direction = L;
            ptShadowRay.TMin      = RAY_EPSILON;
            ptShadowRay.TMax      = dist - 0.002f;
            TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                     0xFF, 0, 0, 1, ptShadowRay, ptShadow);

            if (!ptShadow.isShadowed)
            {
                float  attenuation = 1.0f / max(dist * dist, 0.0001f);
                float3 irradiance  = ptFlux * attenuation;
                radiance += throughput * CookTorranceGGX(N, V, L, albedo, metalness, roughness) * irradiance;
#if PT_HASH_GRID_CACHE_ENABLED
                vertexDirectLighting += CookTorranceGGX(N, V, L, albedo, metalness, roughness) * irradiance;
#endif
#if PT_DENOISE_ENABLED
                float3 ptNEE = throughput * CookTorranceGGX(N, V, L, albedo, metalness, roughness) * irradiance;
                if (bounce == 0u || !isSpecularPath)
                    radianceDiffuse += ptNEE;
                else
                    radianceSpecular += ptNEE;
#endif
            }
        }

        // Sphere light NEE — sample a random point on the sphere surface so
        // neighbouring pixels / frames land on different directions, giving
        // soft shadows after accumulation. Previous implementation sampled
        // the center (hard shadows — indistinguishable from a point light).
        // Uniform-area sampling over the full sphere; a visible-hemisphere
        // importance sampler would converge faster but the math is larger.
        // TASK-67 AC #2.
        for (uint spIdx = 0; spIdx < g_SphereLightCount; spIdx++)
        {
            float3 spCenter     = g_SphereLights[spIdx].position.xyz;
            float  spSphereRad  = g_SphereLights[spIdx].luminousFlux.w;
            float3 spFlux       = g_SphereLights[spIdx].luminousFlux.xyz;

            // Uniform point on unit sphere (Marsaglia's z/azimuth method).
            float2 xiSp = Rand2(rng);
            float  zSp  = 2.0f * xiSp.x - 1.0f;
            float  phi  = 2.0f * PI * xiSp.y;
            float  rSp  = sqrt(max(1.0f - zSp * zSp, 0.0f));
            float3 nLight  = float3(rSp * cos(phi), rSp * sin(phi), zSp);
            float3 pLight  = spCenter + spSphereRad * nLight;

            float3 toLight = pLight - payload.hitPos;
            float  dist    = length(toLight);
            float3 L       = toLight / dist;
            float NdotL    = max(dot(N, L), 0.0f);
            float cosLight = max(dot(nLight, -L), 0.0f);
            if (NdotL <= 0.0f || cosLight <= 0.0f)
                continue;

            ShadowPayload spShadow;
            spShadow.isShadowed = true;
            RayDesc spShadowRay;
            spShadowRay.Origin    = payload.hitPos + N * RAY_EPSILON;
            spShadowRay.Direction = L;
            spShadowRay.TMin      = RAY_EPSILON;
            // Stop just before the sampled surface point — no self-hit on the sphere.
            spShadowRay.TMax      = max(dist - RAY_EPSILON, RAY_EPSILON);
            TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                     0xFF, 0, 0, 1, spShadowRay, spShadow);

            if (!spShadow.isShadowed)
            {
                // Treat spFlux as total radiant flux Φ of a uniform diffuse
                // emitter. Surface radiance L_e = Φ / (π · 4π r²). Area PDF
                // = 1 / (4π r²). Converting to solid-angle at the shading
                // point: E = L_e · cosLight · (4π r²) / dist² · (1/sample).
                // Simplifies to: incomingRadiance = Φ · cosLight / (π · dist²).
                float  geomTerm = cosLight / max(dist * dist, 0.0001f);
                float3 incoming = spFlux * geomTerm / PI;
                radiance += throughput * CookTorranceGGX(N, V, L, albedo, metalness, roughness) * incoming;
#if PT_HASH_GRID_CACHE_ENABLED
                vertexDirectLighting += CookTorranceGGX(N, V, L, albedo, metalness, roughness) * incoming;
#endif
#if PT_DENOISE_ENABLED
                float3 spNEE = throughput * CookTorranceGGX(N, V, L, albedo, metalness, roughness) * incoming;
                if (bounce == 0u || !isSpecularPath)
                    radianceDiffuse += spNEE;
                else
                    radianceSpecular += spNEE;
#endif
            }
        }
