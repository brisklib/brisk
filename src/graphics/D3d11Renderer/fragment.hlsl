float4 tint_unpack4x8unorm(uint param_0) {
  uint j = param_0;
  uint4 i = uint4(j & 0xff, (j >> 8) & 0xff, (j >> 16) & 0xff, j >> 24);
  return float4(i) / 255.0;
}

int tint_ftoi(float v) {
  return ((v <= 2147483520.0f) ? ((v < -2147483648.0f) ? -2147483648 : int(v)) : 2147483647);
}

int2 tint_ftoi_1(float2 v) {
  return ((v <= (2147483520.0f).xx) ? ((v < (-2147483648.0f).xx) ? (-2147483648).xx : int2(v)) : (2147483647).xx);
}

uint tint_ftou(float v) {
  return ((v <= 4294967040.0f) ? ((v < 0.0f) ? 0u : uint(v)) : 4294967295u);
}

uint2 tint_ftou_1(float2 v) {
  return ((v <= (4294967040.0f).xx) ? ((v < (0.0f).xx) ? (0u).xx : uint2(v)) : (4294967295u).xx);
}

struct VertexOutput {
  float4 position;
  float4 data0;
  float4 data1;
  float2 uv;
  float2 canvas_coord;
  uint4 coverage;
};

cbuffer cbuffer_constants : register(b1) {
  uint4 constants[13];
};
cbuffer cbuffer_perFrame : register(b2) {
  uint4 perFrame[3];
};
Texture2D<float4> gradTex_t : register(t8);
Texture2D<float4> fontTex_t : register(t9);
Texture2D<float4> boundTexture_t : register(t10);
SamplerState boundTexture_s : register(s6);

float2 map(float2 p1, float2 p2) {
  return float2(((p1.x * p2.x) + (p1.y * p2.y)), ((p1.x * p2.y) - (p1.y * p2.x)));
}

float4 simpleGradient(float pos) {
  return lerp(asfloat(constants[9]), asfloat(constants[10]), pos);
}

float4 multiGradient(float pos) {
  if ((pos <= 0.0f)) {
    return gradTex_t.Load(uint3(6u, uint(asint(constants[6].x)), 0u));
  }
  if ((pos >= 1.0f)) {
    return gradTex_t.Load(uint3(29u, uint(asint(constants[6].x)), 0u));
  }
  float prev = 0.0f;
  {
    for(uint block = 0u; (block < 6u); block = (block + 1u)) {
      float4 stops4 = gradTex_t.Load(uint3(block, uint(asint(constants[6].x)), 0u));
      {
        for(uint j = 0u; (j < 4u); j = (j + 1u)) {
          uint index = ((block * 4u) + j);
          float curr = stops4[j];
          bool tint_tmp = (pos >= prev);
          if (tint_tmp) {
            tint_tmp = (pos < curr);
          }
          if ((tint_tmp)) {
            float4 c0 = gradTex_t.Load(uint3((6u + max((index - 1u), 0u)), uint(asint(constants[6].x)), 0u));
            float4 c1 = gradTex_t.Load(uint3((6u + index), uint(asint(constants[6].x)), 0u));
            return lerp(c0, c1, ((pos - prev) / ((curr - prev) + 0.00000000099999997172f)));
          }
          prev = curr;
        }
      }
    }
  }
  return (0.0f).xxxx;
}

float3x2 constants_load_2(uint offset) {
  const uint scalar_offset = ((offset + 0u)) / 4;
  uint4 ubo_load = constants[scalar_offset / 4];
  const uint scalar_offset_1 = ((offset + 8u)) / 4;
  uint4 ubo_load_1 = constants[scalar_offset_1 / 4];
  const uint scalar_offset_2 = ((offset + 16u)) / 4;
  uint4 ubo_load_2 = constants[scalar_offset_2 / 4];
  return float3x2(asfloat(((scalar_offset & 2) ? ubo_load.zw : ubo_load.xy)), asfloat(((scalar_offset_1 & 2) ? ubo_load_1.zw : ubo_load_1.xy)), asfloat(((scalar_offset_2 & 2) ? ubo_load_2.zw : ubo_load_2.xy)));
}

float2 transformedTexCoord(float2 uv) {
  uint2 tint_tmp_1;
  boundTexture_t.GetDimensions(tint_tmp_1.x, tint_tmp_1.y);
  float2 tex_size = float2(tint_tmp_1);
  float2 transformed_uv = mul(float3(uv, 1.0f), constants_load_2(112u)).xy;
  if ((asint(constants[8].z) == 0)) {
    transformed_uv = clamp(transformed_uv, (0.5f).xx, (tex_size - 0.5f));
  }
  return (transformed_uv / tex_size);
}

float4 sqr4(float4 x) {
  return (x * x);
}

float2 sqr2(float2 x) {
  return (x * x);
}

float2 conj(float2 xy) {
  return float2(xy.x, -(xy.y));
}

float4 sampleBlur_2d(float2 pos) {
  uint2 tint_tmp_2;
  boundTexture_t.GetDimensions(tint_tmp_2.x, tint_tmp_2.y);
  int2 texSize = int2(tint_tmp_2);
  float sigma = asfloat(constants[8].w);
  int half_size = tint_ftoi(ceil((sigma * 3.0f)));
  float g1 = (1.0f / ((6.283184051513671875f * sigma) * sigma));
  float g2 = (-0.5f / (sigma * sigma));
  float2 w = (1.0f / float2(texSize));
  float2 lo = ((0.5f).xx / float2(texSize));
  float2 hi = (float2((float2(texSize) - 0.5f)) / float2(texSize));
  float tint_symbol_4 = g1;
  float4 tint_symbol_5 = boundTexture_t.Sample(boundTexture_s, pos);
  float4 sum = (tint_symbol_4 * tint_symbol_5);
  {
    for(int i = 1; (i <= half_size); i = (i + 2)) {
      {
        for(int j = 0; (j <= half_size); j = (j + 2)) {
          float tint_symbol_6 = g1;
          float4 tint_symbol_7 = sqr4(float2(float(i), float((i + 1))).xyxy);
          float4 tint_symbol_8 = sqr4(float2(float(j), float((j + 1))).xxyy);
          float4 tint_symbol_9 = exp(((tint_symbol_7 + tint_symbol_8) * g2));
          float4 abcd = (tint_symbol_6 * tint_symbol_9);
          float ab_sum = (abcd[0] + abcd[1]);
          float cd_sum = (abcd[2] + abcd[3]);
          float abcd_sum = (ab_sum + cd_sum);
          float ab_y = (abcd[1] / ab_sum);
          float cd_y = (abcd[3] / cd_sum);
          float abcd_y = (cd_sum / abcd_sum);
          float2 o = float2((((1.0f - abcd_y) * ab_y) + (abcd_y * cd_y)), abcd_y);
          float2 xy = (float2(float(i), float(j)) + o);
          if ((asint(constants[8].z) == 0)) {
            float4 v1 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * xy)), lo, hi));
            float2 tint_symbol_10 = pos;
            float2 tint_symbol_11 = w;
            float2 tint_symbol_12 = conj(xy);
            float2 tint_symbol_13 = clamp((tint_symbol_10 + (tint_symbol_11 * tint_symbol_12.yx)), lo, hi);
            float4 v2 = boundTexture_t.Sample(boundTexture_s, tint_symbol_13);
            float4 v3 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * -(xy))), lo, hi));
            float2 tint_symbol_14 = pos;
            float2 tint_symbol_15 = w;
            float2 tint_symbol_16 = conj(xy.yx);
            float2 tint_symbol_17 = clamp((tint_symbol_14 + (tint_symbol_15 * tint_symbol_16)), lo, hi);
            float4 v4 = boundTexture_t.Sample(boundTexture_s, tint_symbol_17);
            sum = (sum + ((((v1 + v2) + v3) + v4) * abcd_sum));
          } else {
            float4 v1 = boundTexture_t.Sample(boundTexture_s, (pos + (w * xy)));
            float2 tint_symbol_18 = pos;
            float2 tint_symbol_19 = w;
            float2 tint_symbol_20 = conj(xy);
            float4 v2 = boundTexture_t.Sample(boundTexture_s, (tint_symbol_18 + (tint_symbol_19 * tint_symbol_20.yx)));
            float4 v3 = boundTexture_t.Sample(boundTexture_s, (pos + (w * -(xy))));
            float2 tint_symbol_21 = pos;
            float2 tint_symbol_22 = w;
            float2 tint_symbol_23 = conj(xy.yx);
            float4 v4 = boundTexture_t.Sample(boundTexture_s, (tint_symbol_21 + (tint_symbol_22 * tint_symbol_23)));
            sum = (sum + ((((v1 + v2) + v3) + v4) * abcd_sum));
          }
        }
      }
    }
  }
  return sum;
}

float4 sampleBlur_1d(float2 pos, float2 direction) {
  uint2 tint_tmp_3;
  boundTexture_t.GetDimensions(tint_tmp_3.x, tint_tmp_3.y);
  int2 texSize = int2(tint_tmp_3);
  float sigma = asfloat(constants[8].w);
  int half_size = tint_ftoi(ceil((sigma * 3.0f)));
  float g1 = (1.0f / (2.5066280364990234375f * sigma));
  float g2 = (-0.5f / (sigma * sigma));
  float2 w = (1.0f / float2(texSize));
  float2 lo = ((0.5f).xx / float2(texSize));
  float2 hi = (float2((float2(texSize) - 0.5f)) / float2(texSize));
  float tint_symbol_24 = g1;
  float4 tint_symbol_25 = boundTexture_t.Sample(boundTexture_s, pos);
  float4 sum = (tint_symbol_24 * tint_symbol_25);
  {
    for(int i = 1; (i <= half_size); i = (i + 2)) {
      float tint_symbol_26 = g1;
      float2 tint_symbol_27 = sqr2(float2(float(i), float((i + 1))));
      float2 tint_symbol_28 = exp((tint_symbol_27 * g2));
      float2 weights = (tint_symbol_26 * tint_symbol_28);
      float weight_sum = (weights[0] + weights[1]);
      float offset = (weights[1] / weight_sum);
      float2 sample_offset = ((float(i) + offset) * direction);
      if ((asint(constants[8].z) == 0)) {
        float4 v1 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * sample_offset)), lo, hi));
        float4 v2 = boundTexture_t.Sample(boundTexture_s, clamp((pos - (w * sample_offset)), lo, hi));
        sum = (sum + ((v1 + v2) * weight_sum));
      } else {
        float4 v1 = boundTexture_t.Sample(boundTexture_s, (pos + (w * sample_offset)));
        float4 v2 = boundTexture_t.Sample(boundTexture_s, (pos - (w * sample_offset)));
        sum = (sum + ((v1 + v2) * weight_sum));
      }
    }
  }
  return sum;
}

float4 sampleBlur(float2 pos) {
  if ((constants[6].y == 1u)) {
    return sampleBlur_1d(pos, float2(1.0f, 0.0f));
  } else {
    if ((constants[6].y == 2u)) {
      return sampleBlur_1d(pos, float2(0.0f, 1.0f));
    } else {
      return sampleBlur_2d(pos);
    }
  }
}

float positionAlongLine(float2 from_, float2 to, float2 tint_symbol) {
  float2 dir = normalize((to - from_));
  float2 offs = (tint_symbol - from_);
  return (dot(offs, dir) / length((to - from_)));
}

float sdfInfLine(float2 p, float2 p1, float2 p2) {
  float2 dir = normalize((p2 - p1));
  float2 normal = float2(-(dir.y), dir.x);
  return dot((p - p1), normal);
}

float angleGradient(float2 p, float2 p1, float2 p2) {
  float sd = sdfInfLine(p, p1, p2);
  float2 n = map(normalize((p - p1)), normalize((p2 - p1)));
  float first = ((atan2(n.x, n.y) * 0.15915493667125701904f) + 0.75f);
  float second = ((atan2(-(n.x), -(n.y)) * 0.15915493667125701904f) + 0.25f);
  return lerp(first, second, clamp((sd + 0.5f), 0.0f, 1.0f));
}

float gradientPositionForPoint(float2 tint_symbol) {
  if ((asint(constants[12].y) == 0)) {
    return positionAlongLine(asfloat(constants[11].xy), asfloat(constants[11].zw), tint_symbol);
  } else {
    if ((asint(constants[12].y) == 1)) {
      return (length((tint_symbol - asfloat(constants[11].xy))) / length((asfloat(constants[11].zw) - asfloat(constants[11].xy))));
    } else {
      if ((asint(constants[12].y) == 2)) {
        return angleGradient(tint_symbol, asfloat(constants[11].xy), asfloat(constants[11].zw));
      } else {
        if ((asint(constants[12].y) == 3)) {
          float tint_symbol_29 = positionAlongLine(asfloat(constants[11].xy), asfloat(constants[11].zw), tint_symbol);
          float tint_symbol_30 = frac(tint_symbol_29);
          float tint_symbol_31 = abs(((tint_symbol_30 * 2.0f) - 1.0f));
          return (1.0f - tint_symbol_31);
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

float4 computeShadeColor(float2 canvas_coord) {
  float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
  if ((asint(constants[1].y) == -1)) {
    float grad_pos = gradientPosition(canvas_coord);
    if ((asint(constants[6].x) == -1)) {
      result = simpleGradient(grad_pos);
    } else {
      result = multiGradient(grad_pos);
    }
    return result;
  }
  float2 transformed_uv = transformedTexCoord(canvas_coord);
  bool tint_tmp_4 = (asfloat(constants[8].w) > 0.0f);
  if (tint_tmp_4) {
    tint_tmp_4 = (constants[6].y != 0u);
  }
  if ((tint_tmp_4)) {
    result = sampleBlur(transformed_uv);
  } else {
    result = boundTexture_t.Sample(boundTexture_s, transformed_uv);
  }
  result = clamp(result, (0.0f).xxxx, (1.0f).xxxx);
  if ((asint(constants[6].x) != -1)) {
    result = multiGradient(result[asint(constants[6].z)]);
  }
  return result;
}

uint tint_mod(uint lhs, uint rhs) {
  return (lhs % ((rhs == 0u) ? 1u : rhs));
}

uint samplePattern(uint x, uint pattern) {
  return ((pattern >> (tint_mod(x, 12u) & 31u)) & 1u);
}

uint tint_div(uint lhs, uint rhs) {
  return (lhs / ((rhs == 0u) ? 1u : rhs));
}

float atlas(int sprite, int2 pos, uint stride) {
  bool tint_tmp_5 = (pos.x < 0);
  if (!tint_tmp_5) {
    tint_tmp_5 = (pos.x >= int(stride));
  }
  if ((tint_tmp_5)) {
    return 0.0f;
  }
  if ((sprite < 0)) {
    return float((uint((pos.x & pos.y)) & 1u));
  }
  uint tint_symbol_1 = (((uint(sprite) * 8u) + uint(pos.x)) + (uint(pos.y) * stride));
  return fontTex_t.Load(uint3(tint_mod(tint_symbol_1, perFrame[2].x), tint_div(tint_symbol_1, perFrame[2].x), 0u)).r;
}

float4 atlasRGBA(int sprite, int2 pos, uint stride) {
  bool tint_tmp_6 = (pos.x < 0);
  if (!tint_tmp_6) {
    tint_tmp_6 = (((pos.x * 4) + 3) >= int(stride));
  }
  if ((tint_tmp_6)) {
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
  if ((asint(constants[4].z) == 1)) {
    alpha = atlas(sprite, pos, stride);
    return alpha;
  }
  {
    for(int i = 0; (i < asint(constants[4].z)); i = (i + 1)) {
      float tint_symbol_32 = alpha;
      float tint_symbol_33 = atlas(sprite, (pos + int2(i, 0)), stride);
      alpha = (tint_symbol_32 + tint_symbol_33);
    }
  }
  return (alpha / float(asint(constants[4].z)));
}

float3 processSubpixelOutput(float3 rgb) {
  if ((asint(constants[4].w) == 0)) {
    return float3((dot(rgb, float3(0.33329999446868896484f, 0.33340001106262207031f, 0.33329999446868896484f))).xxx);
  } else {
    if ((asint(constants[4].w) == 2)) {
      return rgb.bgr;
    } else {
      return rgb;
    }
  }
}

float3 atlasSubpixel(int sprite, int2 pos, uint stride) {
  if ((asint(constants[4].z) == 6)) {
    float tint_symbol_34 = atlas(sprite, (pos + int2(-2, 0)), stride);
    float tint_symbol_35 = atlas(sprite, (pos + int2(-1, 0)), stride);
    float x0 = (tint_symbol_34 + tint_symbol_35);
    float tint_symbol_36 = atlas(sprite, (pos + (0).xx), stride);
    float tint_symbol_37 = atlas(sprite, (pos + int2(1, 0)), stride);
    float x1 = (tint_symbol_36 + tint_symbol_37);
    float tint_symbol_38 = atlas(sprite, (pos + int2(2, 0)), stride);
    float tint_symbol_39 = atlas(sprite, (pos + int2(3, 0)), stride);
    float x2 = (tint_symbol_38 + tint_symbol_39);
    float tint_symbol_40 = atlas(sprite, (pos + int2(4, 0)), stride);
    float tint_symbol_41 = atlas(sprite, (pos + int2(5, 0)), stride);
    float x3 = (tint_symbol_40 + tint_symbol_41);
    float tint_symbol_42 = atlas(sprite, (pos + int2(6, 0)), stride);
    float tint_symbol_43 = atlas(sprite, (pos + int2(7, 0)), stride);
    float x4 = (tint_symbol_42 + tint_symbol_43);
    float3 filt = float3(0.125f, 0.25f, 0.125f);
    return float3(dot(float3(x0, x1, x2), filt), dot(float3(x1, x2, x3), filt), dot(float3(x2, x3, x4), filt));
  } else {
    if ((asint(constants[4].z) == 3)) {
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
  return (exp(-(((x * x) / ((2.0f * sigma) * sigma)))) / (2.50662827491760253906f * sigma));
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
  float2 tint_symbol_44 = erf(((float2((x).xx) + float2(-(curved), curved)) * (0.70710676908493041992f / sigma)));
  float2 integral = (0.5f + (0.5f * tint_symbol_44));
  return (integral.y - integral.x);
}

float roundedBoxShadow(float2 halfSize, float2 tint_symbol, float sigma, float4 border_radii) {
  float low = (tint_symbol.y - halfSize.y);
  float high = (tint_symbol.y + halfSize.y);
  float start = clamp((-3.0f * sigma), low, high);
  float end = clamp((3.0f * sigma), low, high);
  float step = ((end - start) / 4.0f);
  float y = (start + (step * 0.5f));
  float value = 0.0f;
  uint quadrant = (uint((tint_symbol.x >= 0.0f)) + (2u * uint((tint_symbol.y >= 0.0f))));
  float corner = abs(border_radii[quadrant]);
  {
    for(int i = 0; (i < 4); i = (i + 1)) {
      float tint_symbol_45 = value;
      float tint_symbol_46 = roundedBoxShadowX(tint_symbol.x, (tint_symbol.y - y), sigma, corner, halfSize);
      float tint_symbol_47 = gaussian(y, sigma);
      value = (tint_symbol_45 + ((tint_symbol_46 * tint_symbol_47) * step));
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
  bool tint_tmp_8 = (asint(constants[1].x) == 1);
  if (!tint_tmp_8) {
    tint_tmp_8 = (asint(constants[1].x) == 5);
  }
  bool tint_tmp_7 = (tint_tmp_8);
  if (tint_tmp_7) {
    tint_tmp_7 = (asint(constants[4].w) != 0);
  }
  return (tint_tmp_7);
}

FragOut postprocessColor(FragOut tint_symbol_2, uint2 canvas_coord) {
  FragOut tint_symbol_3 = tint_symbol_2;
  float opacity = asfloat(constants[5].w);
  if ((constants[5].x != 0u)) {
    uint pattern_scale = (constants[5].x >> 24u);
    uint hpattern = (constants[5].x & 4095u);
    uint vpattern = (constants[5].x >> 12u);
    uint tint_symbol_48 = samplePattern(tint_div(canvas_coord.x, pattern_scale), hpattern);
    uint tint_symbol_49 = samplePattern(tint_div(canvas_coord.y, pattern_scale), vpattern);
    uint p = (tint_symbol_48 & tint_symbol_49);
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

float rectangleCoverage(float2 pt, float4 rect) {
  float2 wh = max((0.0f).xx, (min((pt.xy + (0.5f).xx), rect.zw) - max((pt.xy - (0.5f).xx), rect.xy)));
  return (wh.x * wh.y);
}

struct tint_symbol_55 {
  noperspective float4 data0 : TEXCOORD0;
  noperspective float4 data1 : TEXCOORD1;
  noperspective float2 uv : TEXCOORD2;
  noperspective float2 canvas_coord : TEXCOORD3;
  nointerpolation uint4 coverage : TEXCOORD4;
  float4 position : SV_Position;
};
struct tint_symbol_56 {
  float4 color : SV_Target0;
  float4 blend : SV_Target1;
};

FragOut fragmentMain_inner(VertexOutput tint_symbol_2) {
  if ((asint(constants[1].x) == 4)) {
    int2 tex_coord = tint_ftoi_1(tint_symbol_2.position.xy);
    FragOut tint_symbol_57 = {boundTexture_t.Load(int3(tex_coord, 0)), (1.0f).xxxx};
    return tint_symbol_57;
  }
  float4 outColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
  float4 outBlend = float4(0.0f, 0.0f, 0.0f, 0.0f);
  if ((asint(constants[1].x) == 2)) {
    float tint_symbol_50 = roundedBoxShadow((tint_symbol_2.data0.xy * 0.5f), tint_symbol_2.uv, asfloat(constants[8].w), tint_symbol_2.data1);
    outColor = (asfloat(constants[9]) * tint_symbol_50);
  } else {
    bool tint_tmp_9 = (asint(constants[1].x) == 3);
    if (!tint_tmp_9) {
      tint_tmp_9 = (asint(constants[1].x) == 1);
    }
    if ((tint_tmp_9)) {
      int sprite = tint_ftoi(tint_symbol_2.data0.z);
      uint stride = tint_ftou(tint_symbol_2.data0.w);
      int2 tuv = tint_ftoi_1(tint_symbol_2.uv);
      float4 shadeColor = computeShadeColor(tint_symbol_2.canvas_coord);
      if (useBlending()) {
        float3 tint_symbol_51 = atlasSubpixel(sprite, tuv, stride);
        float3 rgb = processSubpixelOutput(tint_symbol_51);
        outColor = (shadeColor * float4(rgb, 1.0f));
        outBlend = float4((shadeColor.a * rgb), 1.0f);
      } else {
        if ((asint(constants[1].x) == 3)) {
          float4 tint_symbol_52 = shadeColor;
          float4 tint_symbol_53 = atlasRGBA(sprite, tuv, stride);
          outColor = (tint_symbol_52 * tint_symbol_53);
        } else {
          float alpha = atlasAccum(sprite, tuv, stride);
          outColor = (shadeColor * float4((alpha).xxxx));
        }
      }
    } else {
      if ((asint(constants[1].x) == 5)) {
        uint2 xy = tint_ftou_1(tint_symbol_2.uv);
        float cov = tint_unpack4x8unorm(tint_symbol_2.coverage[(xy.y & 3u)])[(xy.x & 3u)];
        float4 shadeColor = computeShadeColor(tint_symbol_2.canvas_coord);
        outColor = (shadeColor * float4((cov).xxxx));
      } else {
        if ((asint(constants[1].x) == 0)) {
          float4 shadeColor = computeShadeColor(tint_symbol_2.canvas_coord);
          float4 rect = tint_symbol_2.data0;
          float pixelCoverage = rectangleCoverage(tint_symbol_2.canvas_coord, rect);
          outColor = (pixelCoverage * shadeColor);
        } else {
          outColor = float4(0.0f, 1.0f, 0.0f, 0.5f);
        }
      }
    }
  }
  FragOut tint_symbol_58 = {outColor, outBlend};
  return postprocessColor(tint_symbol_58, tint_ftou_1(tint_symbol_2.canvas_coord));
}

tint_symbol_56 fragmentMain(tint_symbol_55 tint_symbol_54) {
  VertexOutput tint_symbol_59 = {float4(tint_symbol_54.position.xyz, (1.0f / tint_symbol_54.position.w)), tint_symbol_54.data0, tint_symbol_54.data1, tint_symbol_54.uv, tint_symbol_54.canvas_coord, tint_symbol_54.coverage};
  FragOut inner_result = fragmentMain_inner(tint_symbol_59);
  tint_symbol_56 wrapper_result = (tint_symbol_56)0;
  wrapper_result.color = inner_result.color;
  wrapper_result.blend = inner_result.blend;
  return wrapper_result;
}
