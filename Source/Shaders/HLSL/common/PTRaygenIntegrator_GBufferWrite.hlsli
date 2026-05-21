// shadertype=hlsl
// Inline-snippet (NOT a standalone header — see PTRaygenIntegrator.hlsl
// "snippet split" comment). Spliced into RunPathIntegrator's bounce loop
// at the post-payload-read site, after albedo/metalness/roughness are
// in scope and before the sun-NEE block. Verbatim extraction; no semantic
// change. In-scope dependencies: pixel, payload, N, metalness, albedo,
// roughness, bounce, plus globals g_Frame / g_FramePrev / u_PTDenoise_*.

#if PT_DENOISE_ENABLED
        if (bounce == 0u)
        {
            // GBuffer-equivalent write at primary hit. Layout mirrors
            // the rasterizer GBuffer (opaqueGeometryProcessPass.frag) so
            // DecodeGBuffer (common/lightPassCommon.hlsl:38-67) is
            // reusable on these textures. Channel-name constants live in
            // common/PTDenoiseShared.hlsl.
            //
            // RT0.a stores payload.instanceID (TLAS InstanceID() captured
            // in the closest-hit). DecodeGBuffer's sky test reads
            // l_RT0.a == 0; non-sky surfaces here always have a non-zero
            // payload.instanceID + 1 sentinel so instance 0 does not
            // collide with the sky flag.
            u_PTDenoise_PositionInstanceID[pixel] =
                float4(payload.hitPos, float(payload.instanceID + 1u));
            u_PTDenoise_NormalMetalness[pixel] =
                float4(N, metalness);
            u_PTDenoise_AlbedoRoughness[pixel] =
                float4(albedo, roughness);

            // Motion vector at primary hit. Project hitPos with previous
            // frame's view + p_original (held in g_FramePrev), subtract
            // current screen-space position. Sign and unit (pixels) match
            // OpaquePass.frag:124 so SSRCTemporal.comp's
            // `previous_uv = uv + velocity` reprojection is reusable
            // unchanged in CL-2.
            float4 hitWS_curr  = float4(payload.hitPos, 1.0f);
            float4 hitVS_curr  = mul(hitWS_curr, g_Frame.v);
            float4 hitCS_curr  = mul(hitVS_curr, g_Frame.p_original);
            float4 hitVS_prev  = mul(hitWS_curr, g_FramePrev.v);
            float4 hitCS_prev  = mul(hitVS_prev, g_FramePrev.p_original);

            float w_curr = max(abs(hitCS_curr.w), EPSILON);
            float w_prev = max(abs(hitCS_prev.w), EPSILON);
            float2 ndc_curr = hitCS_curr.xy / w_curr;
            float2 ndc_prev = hitCS_prev.xy / w_prev;
            float2 screen_curr = ndc_curr * 0.5f + 0.5f;
            float2 screen_prev = ndc_prev * 0.5f + 0.5f;
            screen_curr.y = 1.0f - screen_curr.y;
            screen_prev.y = 1.0f - screen_prev.y;
            screen_curr *= g_Frame.viewportSize.xy;
            screen_prev *= g_Frame.viewportSize.xy;

            float2 motionVec = screen_prev - screen_curr;
            float  hitDist   = length(payload.hitPos - g_Frame.camera_posWS.xyz);

            u_PTDenoise_MotionHitDist[pixel] = float4(motionVec, hitDist, 0.0f);
        }
#endif
