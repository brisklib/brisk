static bool tint_discarded = false;

uint2 tint_ftou(float2 v) {
  return ((v < (4294967040.0f).xx) ? ((v < (0.0f).xx) ? (0u).xx : uint2(v)) : (4294967295u).xx);
}

int tint_ftoi(float v) {
  return ((v < 2147483520.0f) ? ((v < -2147483648.0f) ? -2147483648 : int(v)) : 2147483647);
}

uint tint_ftou_1(float v) {
  return ((v < 4294967040.0f) ? ((v < 0.0f) ? 0u : uint(v)) : 4294967295u);
}

int2 tint_ftoi_1(float2 v) {
  return ((v < (2147483520.0f).xx) ? ((v < (-2147483648.0f).xx) ? (-2147483648).xx : int2(v)) : (2147483647).xx);
}

struct VertexOutput {
  float4 position;
  float4 data0;
  float4 data1;
  float2 uv;
  float2 canvas_coord;
};

cbuffer cbuffer_constants : register(b1) {
  uint4 constants[15];
};
cbuffer cbuffer_perFrame : register(b2) {
  uint4 perFrame[3];
};
Texture2D<float4> gradTex_t : register(t8);
Texture2D<float4> fontTex_t : register(t9);
Texture2D<float4> boundTexture_t : register(t10);
SamplerState boundTexture_s : register(s6);
SamplerState gradTex_s : register(s7);

float max2(float2 pt) {
  return max(pt.x, pt.y);
}

float2 map(float2 p1, float2 p2) {
  return float2(((p1.x * p2.x) + (p1.y * p2.y)), ((p1.x * p2.y) - (p1.y * p2.x)));
}

float superlength(float2 pt) {
  float p = 4.0f;
  return pow((pow(pt.x, p) + pow(pt.y, p)), (1.0f / p));
}

float sd_length_ex(float2 pt, float border_radius) {
  if ((border_radius == 0.0f)) {
    return max2(abs(pt));
  } else {
    if ((border_radius > 0.0f)) {
      return length(pt);
    } else {
      return superlength(pt);
    }
  }
}

float sd_rectangle(float2 pt, float2 rect_size, float border_radius, int corners) {
  float2 ext = (rect_size * 0.5f);
  uint quadrant = (uint((pt.x >= 0.0f)) + (2u * uint((pt.y >= 0.0f))));
  float rad = abs(border_radius);
  if (((corners & (1 << (quadrant & 31u))) == 0)) {
    rad = 0.0f;
  }
  float2 ext2 = (ext - float2(rad, rad));
  float2 d = (abs(pt) - ext2);
  float tint_symbol_4 = min(max(d.x, d.y), 0.0f);
  float tint_symbol_5 = sd_length_ex(max(d, (0.0f).xx), border_radius);
  return ((tint_symbol_4 + tint_symbol_5) - rad);
}

struct SignedDistance {
  float sd;
  float intersect_sd;
};

SignedDistance signedDistanceArc(float2 pt, float outer_radius, float inner_radius, float start_angle, float end_angle) {
  float outer_d = (length(pt) - outer_radius);
  float inner_d = (length(pt) - inner_radius);
  float circle = max(outer_d, -(inner_d));
  if (((end_angle - start_angle) < 6.28318548202514648438f)) {
    float2 start_sincos = -(float2(cos(start_angle), sin(start_angle)));
    float2 end_sincos = float2(cos(end_angle), sin(end_angle));
    float pie = 0.0f;
    float2 add = float2(dot(pt, start_sincos), dot(pt, end_sincos));
    if (((end_angle - start_angle) > 3.14159274101257324219f)) {
      pie = min(add.x, add.y);
    } else {
      pie = max(add.x, add.y);
    }
    circle = max(circle, pie);
  }
  SignedDistance tint_symbol_40 = {circle, -1000.0f};
  return tint_symbol_40;
}

SignedDistance signedDistanceRectangle(float2 uv, float2 rectSize, float borderRadius, int corners) {
  float sd = 0.0f;
  float intersect_sd = -1000.0f;
  sd = sd_rectangle(uv, rectSize.xy, borderRadius, corners);
  if ((asfloat(constants[14].x) > 0.0f)) {
    int edges = (corners >> 4u);
    if ((edges != 15)) {
      intersect_sd = 1000.0f;
      if (((1 & edges) != 0)) {
        intersect_sd = min(intersect_sd, ((uv.x + (rectSize.x * 0.5f)) - (asfloat(constants[14].x) * 0.5f)));
      }
      if (((2 & edges) != 0)) {
        intersect_sd = min(intersect_sd, ((uv.y + (rectSize.y * 0.5f)) - (asfloat(constants[14].x) * 0.5f)));
      }
      if (((4 & edges) != 0)) {
        intersect_sd = min(intersect_sd, ((-(uv.x) + (rectSize.x * 0.5f)) - (asfloat(constants[14].x) * 0.5f)));
      }
      if (((8 & edges) != 0)) {
        intersect_sd = min(intersect_sd, ((-(uv.y) + (rectSize.y * 0.5f)) - (asfloat(constants[14].x) * 0.5f)));
      }
      if (((3 & edges) == 3)) {
        intersect_sd = min(intersect_sd, max(uv.x, uv.y));
      }
      if (((6 & edges) == 6)) {
        intersect_sd = min(intersect_sd, max(-(uv.x), uv.y));
      }
      if (((9 & edges) == 9)) {
        intersect_sd = min(intersect_sd, max(uv.x, -(uv.y)));
      }
      if (((12 & edges) == 12)) {
        intersect_sd = min(intersect_sd, max(-(uv.x), -(uv.y)));
      }
    }
  }
  SignedDistance tint_symbol_41 = {sd, intersect_sd};
  return tint_symbol_41;
}

struct Colors {
  float4 brush;
  float4 stroke;
};

float4 gammaColor(float4 color) {
  return color;
}

float4 simpleGradient(float pos, bool stroke) {
  float4 color1 = (stroke ? asfloat(constants[11]) : asfloat(constants[9]));
  float4 color2 = (stroke ? asfloat(constants[12]) : asfloat(constants[10]));
  float4 tint_symbol_6 = gammaColor(color1);
  float4 tint_symbol_7 = gammaColor(color2);
  return lerp(tint_symbol_6, tint_symbol_7, pos);
}

float4 multiGradient(float pos) {
  uint2 tint_tmp;
  gradTex_t.GetDimensions(tint_tmp.x, tint_tmp.y);
  float2 invDims = ((1.0f).xx / float2(tint_tmp));
  float4 tint_symbol_8 = gradTex_t.Sample(gradTex_s, (float2((0.5f + (pos * 1023.0f)), (0.5f + float(asint(constants[6].x)))) * invDims));
  return gammaColor(tint_symbol_8);
}

float4 remixColors(float4 value) {
  return ((((asfloat(constants[9]) * value.x) + (asfloat(constants[10]) * value.y)) + (asfloat(constants[11]) * value.z)) + (asfloat(constants[12]) * value.w));
}

float2 transformedTexCoord(float2 uv) {
  float3x2 texture_matrix = float3x2(float2(asfloat(constants[7].x), asfloat(constants[7].y)), float2(asfloat(constants[7].z), asfloat(constants[7].w)), float2(asfloat(constants[8].x), asfloat(constants[8].y)));
  uint2 tint_tmp_1;
  boundTexture_t.GetDimensions(tint_tmp_1.x, tint_tmp_1.y);
  float2 tex_size = float2(tint_tmp_1);
  float2 transformed_uv = mul(float3(uv, 1.0f), texture_matrix).xy;
  if ((asint(constants[8].z) == 0)) {
    transformed_uv = clamp(transformed_uv, (0.5f).xx, (tex_size - 0.5f));
  }
  return (transformed_uv / tex_size);
}

float positionAlongLine(float2 from_, float2 to, float2 tint_symbol) {
  float2 dir = normalize((to - from_));
  float2 offs = (tint_symbol - from_);
  return (dot(offs, dir) / length((to - from_)));
}

float3 mapLine(float2 from_, float2 to, float2 tint_symbol) {
  float len = length((to - from_));
  float2 dir = normalize((to - from_));
  float2 tint_symbol_10 = map((tint_symbol - from_), dir);
  return float3(tint_symbol_10, len);
}

float getAngle(float2 x) {
  return ((atan2(x.y, -(x.x)) / 6.28318548202514648438f) + 0.5f);
}

float gradientPositionForPoint(float2 tint_symbol) {
  if ((asint(constants[14].y) == 0)) {
    return positionAlongLine(asfloat(constants[13].xy), asfloat(constants[13].zw), tint_symbol);
  } else {
    if ((asint(constants[14].y) == 1)) {
      return (length((tint_symbol - asfloat(constants[13].xy))) / length((asfloat(constants[13].zw) - asfloat(constants[13].xy))));
    } else {
      if ((asint(constants[14].y) == 2)) {
        float3 tint_symbol_11 = mapLine(asfloat(constants[13].xy), asfloat(constants[13].zw), tint_symbol);
        return getAngle(tint_symbol_11.xy);
      } else {
        if ((asint(constants[14].y) == 3)) {
          float tint_symbol_12 = positionAlongLine(asfloat(constants[13].xy), asfloat(constants[13].zw), tint_symbol);
          float tint_symbol_13 = frac(tint_symbol_12);
          float tint_symbol_14 = abs(((tint_symbol_13 * 2.0f) - 1.0f));
          return (1.0f - tint_symbol_14);
        } else {
          return 0.5f;
        }
      }
    }
  }
}

float gradientPosition(float2 canvas_coord) {
  float pos = gradientPositionForPoint(canvas_coord);
  return clamp(pos, 0.0f, 1.0f);
}

Colors simpleCalcColors(float2 canvas_coord) {
  Colors result = (Colors)0;
  float grad_pos = gradientPosition(canvas_coord);
  if ((asint(constants[6].x) == -1)) {
    result.brush = simpleGradient(grad_pos, false);
  } else {
    result.brush = multiGradient(grad_pos);
  }
  result.stroke = simpleGradient(grad_pos, true);
  return result;
}

Colors calcColors(float2 canvas_coord) {
  Colors result = (Colors)0;
  if ((asint(constants[1].y) != -1)) {
    float2 transformed_uv = transformedTexCoord(canvas_coord);
    float4 tint_symbol_9 = boundTexture_t.Sample(boundTexture_s, transformed_uv);
    result.brush = gammaColor(tint_symbol_9);
    result.brush = clamp(result.brush, (0.0f).xxxx, (1.0f).xxxx);
    if ((asint(constants[6].x) == -10)) {
      result.brush = remixColors(result.brush);
    } else {
      if ((asint(constants[6].x) != -1)) {
        result.brush = multiGradient(result.brush[asint(constants[6].z)]);
      }
    }
    result.stroke = (0.0f).xxxx;
  } else {
    result = simpleCalcColors(canvas_coord);
  }
  return result;
}

float toCoverage(float sd) {
  return clamp((0.5f - sd), 0.0f, 1.0f);
}

float4 fillOnly(float signed_distance, Colors colors) {
  float alpha = toCoverage(signed_distance);
  return (colors.brush * alpha);
}

float4 fillAndStroke(float stroke_signed_distance, float mask_signed_distance, Colors colors) {
  float border_alpha = toCoverage(stroke_signed_distance);
  float mask_alpha = toCoverage(mask_signed_distance);
  return (lerp(colors.brush, colors.stroke, border_alpha) * mask_alpha);
}

float4 signedDistanceToColor(SignedDistance sd, float2 canvas_coord, float2 uv, float2 rectSize) {
  Colors colors = calcColors(canvas_coord);
  if ((asfloat(constants[14].x) > 0.0f)) {
    float stroke_sd = -(((asfloat(constants[14].x) * 0.5f) + sd.sd));
    stroke_sd = max(stroke_sd, sd.intersect_sd);
    return fillAndStroke(stroke_sd, (sd.sd - (asfloat(constants[14].x) * 0.5f)), colors);
  } else {
    return fillOnly(sd.sd, colors);
  }
}

uint tint_mod(uint lhs, uint rhs) {
  return (lhs % ((rhs == 0u) ? 1u : rhs));
}

uint get_pattern(uint x, uint pattern) {
  return ((pattern >> (tint_mod(x, 24u) & 31u)) & 1u);
}

float mask(float2 pt) {
  float2 scissor_size = (asfloat(constants[5]).zw - asfloat(constants[5]).xy);
  float tint_symbol_15 = sd_rectangle((pt - (asfloat(constants[5]).xy + (scissor_size * 0.5f))), scissor_size, asfloat(constants[1].z), asint(constants[1].w));
  return toCoverage(tint_symbol_15);
}

uint tint_div(uint lhs, uint rhs) {
  return (lhs / ((rhs == 0u) ? 1u : rhs));
}

float atlas(int sprite, int2 pos, uint stride) {
  bool tint_tmp_2 = (pos.x < 0);
  if (!tint_tmp_2) {
    tint_tmp_2 = (pos.x >= int(stride));
  }
  if ((tint_tmp_2)) {
    return 0.0f;
  }
  if ((sprite < 0)) {
    return float((uint((pos.x & pos.y)) & 1u));
  }
  uint tint_symbol_1 = (((uint(sprite) * 8u) + uint(pos.x)) + (uint(pos.y) * stride));
  return fontTex_t.Load(uint3(tint_mod(tint_symbol_1, perFrame[2].x), tint_div(tint_symbol_1, perFrame[2].x), 0u)).r;
}

float4 atlasRGBA(int sprite, int2 pos, uint stride) {
  bool tint_tmp_3 = (pos.x < 0);
  if (!tint_tmp_3) {
    tint_tmp_3 = (((pos.x * 4) + 3) >= int(stride));
  }
  if ((tint_tmp_3)) {
    return (0.0f).xxxx;
  }
  if ((sprite < 0)) {
    return float4((float((uint((pos.x & pos.y)) & 1u))).xxxx);
  }
  uint tint_symbol_1 = (((uint(sprite) * 8u) + (uint(pos.x) * 4u)) + (uint(pos.y) * stride));
  uint2 c = uint2(tint_mod(tint_symbol_1, perFrame[2].x), tint_div(tint_symbol_1, perFrame[2].x));
  return float4(fontTex_t.Load(uint3(c, 0u)).r, fontTex_t.Load(uint3((c + uint2(1u, 0u)), 0u)).r, fontTex_t.Load(uint3((c + uint2(2u, 0u)), 0u)).r, fontTex_t.Load(uint3((c + uint2(3u, 0u)), 0u)).r);
}

float atlasAccum(int sprite, int2 pos, uint stride) {
  float alpha = 0.0f;
  if ((asint(constants[3].z) == 1)) {
    alpha = atlas(sprite, pos, stride);
    return alpha;
  }
  {
    for(int i = 0; (i < asint(constants[3].z)); i = (i + 1)) {
      float tint_symbol_16 = alpha;
      float tint_symbol_17 = atlas(sprite, (pos + int2(i, 0)), stride);
      alpha = (tint_symbol_16 + tint_symbol_17);
    }
  }
  return (alpha / float(asint(constants[3].z)));
}

float3 atlasSubpixel(int sprite, int2 pos, uint stride) {
  if ((asint(constants[3].z) == 6)) {
    float tint_symbol_18 = atlas(sprite, (pos + int2(-2, 0)), stride);
    float tint_symbol_19 = atlas(sprite, (pos + int2(-1, 0)), stride);
    float x0 = (tint_symbol_18 + tint_symbol_19);
    float tint_symbol_20 = atlas(sprite, (pos + (0).xx), stride);
    float tint_symbol_21 = atlas(sprite, (pos + int2(1, 0)), stride);
    float x1 = (tint_symbol_20 + tint_symbol_21);
    float tint_symbol_22 = atlas(sprite, (pos + int2(2, 0)), stride);
    float tint_symbol_23 = atlas(sprite, (pos + int2(3, 0)), stride);
    float x2 = (tint_symbol_22 + tint_symbol_23);
    float tint_symbol_24 = atlas(sprite, (pos + int2(4, 0)), stride);
    float tint_symbol_25 = atlas(sprite, (pos + int2(5, 0)), stride);
    float x3 = (tint_symbol_24 + tint_symbol_25);
    float tint_symbol_26 = atlas(sprite, (pos + int2(6, 0)), stride);
    float tint_symbol_27 = atlas(sprite, (pos + int2(7, 0)), stride);
    float x4 = (tint_symbol_26 + tint_symbol_27);
    float3 filt = float3(0.125f, 0.25f, 0.125f);
    return float3(dot(float3(x0, x1, x2), filt), dot(float3(x1, x2, x3), filt), dot(float3(x2, x3, x4), filt));
  } else {
    if ((asint(constants[3].z) == 3)) {
      float x0 = atlas(sprite, (pos + int2(-2, 0)), stride);
      float x1 = atlas(sprite, (pos + int2(-1, 0)), stride);
      float x2 = atlas(sprite, (pos + (0).xx), stride);
      float x3 = atlas(sprite, (pos + int2(1, 0)), stride);
      float x4 = atlas(sprite, (pos + int2(2, 0)), stride);
      float x5 = atlas(sprite, (pos + int2(3, 0)), stride);
      float x6 = atlas(sprite, (pos + int2(4, 0)), stride);
      return float3((((((x0 * 0.03125f) + (x1 * 0.30078125f)) + (x2 * 0.3359375f)) + (x3 * 0.30078125f)) + (x4 * 0.03125f)), (((((x1 * 0.03125f) + (x2 * 0.30078125f)) + (x3 * 0.3359375f)) + (x4 * 0.30078125f)) + (x5 * 0.03125f)), (((((x2 * 0.03125f) + (x3 * 0.30078125f)) + (x4 * 0.3359375f)) + (x5 * 0.30078125f)) + (x6 * 0.03125f)));
    } else {
      return (1.0f).xxx;
    }
  }
}

float gaussian(float x, float sigma) {
  float pi = 3.14159274101257324219f;
  return (exp(-(((x * x) / ((2.0f * sigma) * sigma)))) / (sqrt((2.0f * pi)) * sigma));
}

float2 erf(float2 x) {
  float2 s = float2(sign(x));
  float2 a = abs(x);
  float2 x1 = (1.0f + ((0.27839300036430358887f + ((0.23038899898529052734f + (0.07810799777507781982f * (a * a))) * a)) * a));
  float2 x2 = (x1 * x1);
  return (s - (s / (x2 * x2)));
}

float roundedBoxShadowX(float x, float y, float sigma, float corner, float2 halfSize) {
  float delta = min(((halfSize.y - corner) - abs(y)), 0.0f);
  float curved = ((halfSize.x - corner) + sqrt(max(0.0f, ((corner * corner) - (delta * delta)))));
  float2 tint_symbol_28 = erf(((float2((x).xx) + float2(-(curved), curved)) * (0.70710676908493041992f / sigma)));
  float2 integral = (0.5f + (0.5f * tint_symbol_28));
  return (integral.y - integral.x);
}

float roundedBoxShadow(float2 halfSize, float2 tint_symbol, float sigma, float corner) {
  float low = (tint_symbol.y - halfSize.y);
  float high = (tint_symbol.y + halfSize.y);
  float start = clamp((-3.0f * sigma), low, high);
  float end = clamp((3.0f * sigma), low, high);
  float step = ((end - start) / 4.0f);
  float y = (start + (step * 0.5f));
  float value = 0.0f;
  {
    for(int i = 0; (i < 4); i = (i + 1)) {
      float tint_symbol_29 = value;
      float tint_symbol_30 = roundedBoxShadowX(tint_symbol.x, (tint_symbol.y - y), sigma, corner, halfSize);
      float tint_symbol_31 = gaussian(y, sigma);
      value = (tint_symbol_29 + ((tint_symbol_30 * tint_symbol_31) * step));
      y = (y + step);
    }
  }
  return value;
}

float4 applyGamma(float4 tint_symbol_2, float gamma) {
  return pow(max(tint_symbol_2, (0.0f).xxxx), float4((gamma).xxxx));
}

float4 applyBlueLightFilter(float4 tint_symbol_2, float intensity) {
  return (tint_symbol_2 * float4(1.0f, (1.0f - ((intensity * 0.60000002384185791016f) * 0.60000002384185791016f)), (1.0f - (intensity * 0.60000002384185791016f)), 1.0f));
}

struct FragOut {
  float4 color;
  float4 blend;
};

bool useBlending() {
  bool tint_tmp_5 = (asint(constants[1].x) == 2);
  if (!tint_tmp_5) {
    tint_tmp_5 = (asint(constants[1].x) == 4);
  }
  bool tint_tmp_4 = (tint_tmp_5);
  if (tint_tmp_4) {
    tint_tmp_4 = (asint(constants[3].w) != 0);
  }
  return (tint_tmp_4);
}

FragOut postprocessColor(FragOut tint_symbol_2, float mask_value, uint2 canvas_coord) {
  FragOut tint_symbol_3 = tint_symbol_2;
  float opacity = (asfloat(constants[4].w) * mask_value);
  if (((constants[4].x | constants[4].y) != 0u)) {
    uint tint_symbol_32 = get_pattern(tint_div(canvas_coord.x, uint(asint(constants[4].z))), constants[4].x);
    uint tint_symbol_33 = get_pattern(tint_div(canvas_coord.y, uint(asint(constants[4].z))), constants[4].y);
    uint p = (tint_symbol_32 & tint_symbol_33);
    opacity = (opacity * float(p));
  }
  tint_symbol_3.color = (tint_symbol_3.color * opacity);
  if (useBlending()) {
    tint_symbol_3.blend = (tint_symbol_3.blend * opacity);
  }
  if ((asfloat(perFrame[1].x) != 0.0f)) {
    tint_symbol_3.color = applyBlueLightFilter(tint_symbol_3.color, asfloat(perFrame[1].x));
    if (useBlending()) {
      tint_symbol_3.blend = applyBlueLightFilter(tint_symbol_3.blend, asfloat(perFrame[1].x));
    }
  }
  if ((asfloat(perFrame[1].y) != 1.0f)) {
    tint_symbol_3.color = applyGamma(tint_symbol_3.color, asfloat(perFrame[1].y));
    if (useBlending()) {
      tint_symbol_3.blend = applyGamma(tint_symbol_3.blend, asfloat(perFrame[1].y));
    }
  }
  if (!(useBlending())) {
    tint_symbol_3.blend = float4((tint_symbol_3.color.a).xxxx);
  }
  return tint_symbol_3;
}

struct tint_symbol_38 {
  noperspective float4 data0 : TEXCOORD0;
  noperspective float4 data1 : TEXCOORD1;
  noperspective float2 uv : TEXCOORD2;
  noperspective float2 canvas_coord : TEXCOORD3;
  float4 position : SV_Position;
};
struct tint_symbol_39 {
  float4 color : SV_Target0;
  float4 blend : SV_Target1;
};

FragOut fragmentMain_inner(VertexOutput tint_symbol_2) {
  float2 pt = float2(0.0f, 0.0f);
  if ((asint(constants[6].w) != 0)) {
    pt = tint_symbol_2.position.xy;
  } else {
    pt = tint_symbol_2.canvas_coord;
  }
  float mask_value = mask(pt);
  if ((mask_value <= 0.0f)) {
    tint_discarded = true;
  }
  float4 outColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
  float4 outBlend = float4(0.0f, 0.0f, 0.0f, 0.0f);
  bool useBlend = false;
  bool tint_tmp_6 = (asint(constants[1].x) == 0);
  if (!tint_tmp_6) {
    tint_tmp_6 = (asint(constants[1].x) == 1);
  }
  if ((tint_tmp_6)) {
    SignedDistance sd = (SignedDistance)0;
    if ((asint(constants[1].x) == 0)) {
      sd = signedDistanceRectangle(tint_symbol_2.uv, tint_symbol_2.data0.xy, tint_symbol_2.data1.y, tint_ftoi(tint_symbol_2.data1.z));
    } else {
      sd = signedDistanceArc(tint_symbol_2.uv, tint_symbol_2.data0.z, tint_symbol_2.data0.w, tint_symbol_2.data1.x, tint_symbol_2.data1.y);
    }
    outColor = signedDistanceToColor(sd, tint_symbol_2.canvas_coord, tint_symbol_2.uv, tint_symbol_2.data0.xy);
  } else {
    if ((asint(constants[1].x) == 3)) {
      float tint_symbol_34 = roundedBoxShadow((tint_symbol_2.data0.xy * 0.5f), tint_symbol_2.uv, (asfloat(constants[14].x) * 0.18000000715255737305f), tint_symbol_2.data1.y);
      outColor = (asfloat(constants[9]) * tint_symbol_34);
    } else {
      bool tint_tmp_8 = (asint(constants[1].x) == 4);
      if (!tint_tmp_8) {
        tint_tmp_8 = (asint(constants[1].x) == 5);
      }
      bool tint_tmp_7 = (tint_tmp_8);
      if (!tint_tmp_7) {
        tint_tmp_7 = (asint(constants[1].x) == 2);
      }
      if ((tint_tmp_7)) {
        int sprite = tint_ftoi(tint_symbol_2.data0.z);
        uint stride = tint_ftou_1(tint_symbol_2.data0.w);
        int2 tuv = tint_ftoi_1(tint_symbol_2.uv);
        Colors colors = calcColors(tint_symbol_2.canvas_coord);
        if (useBlending()) {
          float3 rgb = atlasSubpixel(sprite, tuv, stride);
          outColor = (colors.brush * float4(rgb, 1.0f));
          outBlend = float4((colors.brush.a * rgb), 1.0f);
        } else {
          if ((asint(constants[1].x) == 5)) {
            float4 tint_symbol_35 = colors.brush;
            float4 tint_symbol_36 = atlasRGBA(sprite, tuv, stride);
            outColor = (tint_symbol_35 * tint_symbol_36);
          } else {
            float alpha = atlasAccum(sprite, tuv, stride);
            outColor = (colors.brush * float4((alpha).xxxx));
          }
        }
      }
    }
  }
  FragOut tint_symbol_42 = {outColor, outBlend};
  return postprocessColor(tint_symbol_42, mask_value, tint_ftou(tint_symbol_2.canvas_coord));
}

tint_symbol_39 fragmentMain(tint_symbol_38 tint_symbol_37) {
  VertexOutput tint_symbol_43 = {float4(tint_symbol_37.position.xyz, (1.0f / tint_symbol_37.position.w)), tint_symbol_37.data0, tint_symbol_37.data1, tint_symbol_37.uv, tint_symbol_37.canvas_coord};
  FragOut inner_result = fragmentMain_inner(tint_symbol_43);
  tint_symbol_39 wrapper_result = (tint_symbol_39)0;
  wrapper_result.color = inner_result.color;
  wrapper_result.blend = inner_result.blend;
  if (tint_discarded) {
    discard;
  }
  return wrapper_result;
}
