1

function [F] = fr_F_Schlick(f0, f90, u)
	F = f0 + (f90 - f0) * transpose(power(1.0 - u, 5.0));
endfunction

function [D] = Frostbite_D_GGX(NdotH, roughness)
  a = power(roughness, 2.0);
	a2 = power(a, 2.0);
	f = power(NdotH, 2.0) * (transpose(a2) - 1.0) + 1.0;
	D = 1./power(f, 2.0);
  D = D * a2;
endfunction

function [G_1] = GeometrySchlickGGX(NdotV, roughness)
    a = roughness;
    k = power(a, 2.0);
    nom   = NdotV;
    denom = NdotV * transpose((1.0 - k)) + k;   
    G_1 = transpose(nom) / denom;
endfunction

function [G_2] = GeometrySmith(NdotV, NdotL, roughness)
    ggx2 = GeometrySchlickGGX(NdotV, roughness);
    ggx1 = GeometrySchlickGGX(NdotL, roughness);
    G_2 = ggx1 * ggx2;
endfunction

tx = ty = linspace (0, 1, 64)';
[xx, yy] = meshgrid (tx, ty);
tz = fr_F_Schlick(tx, 1.0, ty);
mesh (tx, ty, tz);