1

function [Diff] = Diff_Lambert(NdotV, NdotL, albedo)
  #Lambert diffuse BRDF is view-independent
  Diff = ones(size(NdotV, 1), size(NdotL, 1));
	Diff *= albedo / pi;
  Diff = transpose(Diff);
endfunction

function [Diff] = Diff_OrenNayar(NdotV, NdotL, albedo, roughness)
  a = roughness;
  A = 1.0 - (a ./ (2.0 * a + 0.66));
  B = 0.45 * (a ./ (a + 0.09))
  Diff = ones(size(NdotV, 1), size(NdotL, 1));
	Diff *= albedo / pi;
  Diff = transpose(Diff);
endfunction

function [F] = F_Schlick(f0, u)
	F = transpose(f0 + (1.0 - f0) * transpose(power(1.0 - u, 5.0)));
endfunction

function [D] = D_GGX(NdotH, roughness)
  a = power(roughness, 2.0);
	a2 = power(a, 2.0);
	f = power(NdotH, 2.0) * transpose(a2 - 1.0) + 1.0;
  f = power(f, 2.0);
  f = 1./ (f + eps);
	D = repmat(a2, size(a2, 2), size(NdotH, 1)) .* transpose(f);
endfunction

function [G] = G_SchlickGGX(NdotV, roughness)
    a = roughness;
    k = power(a, 2.0);
    nom   = NdotV;
    denom = NdotV * transpose((1.0 - k)) + transpose(repmat(k, size(k, 2), size(NdotV, 1)));
    denom = 1./ (denom + eps);
    G = transpose(repmat(nom, size(nom, 2), size(k, 1)) .* denom);
endfunction

function [V] = V_Smith(NdotV, NdotL, roughness)
    ggx2 = G_SchlickGGX(NdotV, roughness);
    ggx1 = G_SchlickGGX(NdotL, roughness);
    V = ggx1 .* ggx2;
endfunction

tx = linspace (0, 1, 32)';
ty = linspace (0, 1, 31)';
[xx, yy] = meshgrid (tx, ty);
D = D_GGX(tx, ty);
V = V_Smith(tx, tx, ty);
F = F_Schlick(1.0, tx);
F = repmat(F, size(F, 2), size(ty, 1));
tz = Diff_Lambert(tx, ty, 1.0);
mesh (tx, ty, tz);
xlabel ("NdotV");
ylabel ("NdotL");
zlabel ("Directional Albedo");