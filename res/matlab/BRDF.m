1

function [F] = F_Schlick(f0, f90, u)
	F = repmat(f0, size(f0, 2), size(f0, 1)) + (f90 - f0) * transpose(power(1.0 - u, 5.0));
endfunction

function [D] = D_GGX(NdotH, roughness)
  a = power(roughness, 2.0);
	a2 = power(a, 2.0);
	f = power(NdotH, 2.0) * (transpose(a2) - 1.0) + 1.0;
	D = repmat(a2, size(a2, 2), size(a2, 1)) * transpose(1./power(f, 2.0));
endfunction

function [G] = G_SchlickGGX(NdotV, roughness)
    a = roughness;
    k = power(a, 2.0);
    nom   = NdotV;
    denom = NdotV * transpose((1.0 - k)) + repmat(k, size(k, 2), size(k, 1));
    G = repmat(nom, size(nom, 2), size(nom, 1)) * transpose(1./ denom);
endfunction

function [V] = V_Smith(NdotV, NdotL, roughness)
    ggx2 = G_SchlickGGX(NdotV, roughness);
    ggx1 = G_SchlickGGX(NdotL, roughness);
    V = ggx1 * transpose(ggx2);
endfunction

tx = ty = linspace (0, 1, 64)';
[xx, yy] = meshgrid (tx, ty);
tz = D_GGX(tx, ty);
mesh (tx, ty, tz);