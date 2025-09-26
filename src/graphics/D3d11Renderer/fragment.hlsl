float4 tint_unpack4x8unorm(uint param_0) {
  uint j = param_0;
  uint4 i = uint4(j & 0xff, (j >> 8) & 0xff, (j >> 16) & 0xff, j >> 24);
  return float4(i) / 255.0;
}

int tint_ftoi(float v) {
  return ((v <= 2147483520.0f) ? ((v < -2147483648.0f) ? -2147483648 : int(v)) : 2147483647);
}

uint2 tint_ftou(float2 v) {
  return ((v <= (4294967040.0f).xx) ? ((v < (0.0f).xx) ? (0u).xx : uint2(v)) : (4294967295u).xx);
}

int2 tint_ftoi_1(float2 v) {
  return ((v <= (2147483520.0f).xx) ? ((v < (-2147483648.0f).xx) ? (-2147483648).xx : int2(v)) : (2147483647).xx);
}

uint tint_ftou_1(float v) {
  return ((v <= 4294967040.0f) ? ((v < 0.0f) ? 0u : uint(v)) : 4294967295u);
}

struct VertexOutput {
  float4 position;
  float4 data0;
  float4 data1;
  float2 uv;
  float2 canvas_coord;
  uint4 coverage;
};

float3 screen(float3 cb, float3 cs) {
  return ((cb + cs) - (cb * cs));
}

float color_dodge(float cb, float cs) {
  if ((cb == 0.0f)) {
    return 0.0f;
  } else {
    if ((cs == 1.0f)) {
      return 1.0f;
    } else {
      return min(1.0f, (cb / (1.0f - cs)));
    }
  }
}

float color_burn(float cb, float cs) {
  if ((cb == 1.0f)) {
    return 1.0f;
  } else {
    if ((cs == 0.0f)) {
      return 0.0f;
    } else {
      return (1.0f - min(1.0f, ((1.0f - cb) / cs)));
    }
  }
}

float3 hard_light(float3 cb, float3 cs) {
  float3 tint_symbol_5 = screen(cb, ((2.0f * cs) - 1.0f));
  return ((cs <= (0.5f).xxx) ? ((cb * 2.0f) * cs) : tint_symbol_5);
}

float3 soft_light(float3 cb, float3 cs) {
  float3 d = ((cb <= (0.25f).xxx) ? (((((16.0f * cb) - 12.0f) * cb) + 4.0f) * cb) : sqrt(cb));
  return ((cs <= (0.5f).xxx) ? (cb - (((1.0f - (2.0f * cs)) * cb) * (1.0f - cb))) : (cb + (((2.0f * cs) - 1.0f) * (d - cb))));
}

float sat(float3 c) {
  return (max(c.x, max(c.y, c.z)) - min(c.x, min(c.y, c.z)));
}

float lum(float3 c) {
  float3 f = float3(0.30000001192092895508f, 0.58999997377395629883f, 0.10999999940395355225f);
  return dot(c, f);
}

float3 clip_color(float3 c_in) {
  float3 c = c_in;
  float l = lum(c);
  float n = min(c.x, min(c.y, c.z));
  float x = max(c.x, max(c.y, c.z));
  if ((n < 0.0f)) {
    c = (l + (((c - l) * l) / (l - n)));
  }
  if ((x > 1.0f)) {
    c = (l + (((c - l) * (1.0f - l)) / (x - l)));
  }
  return c;
}

float3 set_lum(float3 c, float l) {
  float3 tint_symbol_6 = c;
  float tint_symbol_7 = l;
  float tint_symbol_8 = lum(c);
  return clip_color((tint_symbol_6 + (tint_symbol_7 - tint_symbol_8)));
}

void set_sat_inner(inout float cmin, inout float cmid, inout float cmax, float s) {
  if ((cmax > cmin)) {
    cmid = (((cmid - cmin) * s) / (cmax - cmin));
    cmax = s;
  } else {
    cmid = 0.0f;
    cmax = 0.0f;
  }
  cmin = 0.0f;
}

float3 set_sat(float3 c, float s) {
  float r = c.r;
  float g = c.g;
  float b = c.b;
  if ((r <= g)) {
    if ((g <= b)) {
      set_sat_inner(r, g, b, s);
    } else {
      if ((r <= b)) {
        set_sat_inner(r, b, g, s);
      } else {
        set_sat_inner(b, r, g, s);
      }
    }
  } else {
    if ((r <= b)) {
      set_sat_inner(g, r, b, s);
    } else {
      if ((g <= b)) {
        set_sat_inner(g, b, r, s);
      } else {
        set_sat_inner(b, g, r, s);
      }
    }
  }
  return float3(r, g, b);
}

float3 blend_mix(float3 cb, float3 cs, uint mode) {
  float3 b = (0.0f).xxx;
  switch(mode) {
    case 1u: {
      b = (cb * cs);
      break;
    }
    case 2u: {
      b = screen(cb, cs);
      break;
    }
    case 3u: {
      b = hard_light(cs, cb);
      break;
    }
    case 4u: {
      b = min(cb, cs);
      break;
    }
    case 5u: {
      b = max(cb, cs);
      break;
    }
    case 6u: {
      float tint_symbol_9 = color_dodge(cb.x, cs.x);
      float tint_symbol_10 = color_dodge(cb.y, cs.y);
      float tint_symbol_11 = color_dodge(cb.z, cs.z);
      b = float3(tint_symbol_9, tint_symbol_10, tint_symbol_11);
      break;
    }
    case 7u: {
      float tint_symbol_12 = color_burn(cb.x, cs.x);
      float tint_symbol_13 = color_burn(cb.y, cs.y);
      float tint_symbol_14 = color_burn(cb.z, cs.z);
      b = float3(tint_symbol_12, tint_symbol_13, tint_symbol_14);
      break;
    }
    case 8u: {
      b = hard_light(cb, cs);
      break;
    }
    case 9u: {
      b = soft_light(cb, cs);
      break;
    }
    case 10u: {
      b = abs((cb - cs));
      break;
    }
    case 11u: {
      b = ((cb + cs) - ((2.0f * cb) * cs));
      break;
    }
    case 12u: {
      float3 tint_symbol_15 = cs;
      float tint_symbol_16 = sat(cb);
      float3 tint_symbol_17 = set_sat(tint_symbol_15, tint_symbol_16);
      float tint_symbol_18 = lum(cb);
      b = set_lum(tint_symbol_17, tint_symbol_18);
      break;
    }
    case 13u: {
      float3 tint_symbol_19 = cb;
      float tint_symbol_20 = sat(cs);
      float3 tint_symbol_21 = set_sat(tint_symbol_19, tint_symbol_20);
      float tint_symbol_22 = lum(cb);
      b = set_lum(tint_symbol_21, tint_symbol_22);
      break;
    }
    case 14u: {
      float3 tint_symbol_23 = cs;
      float tint_symbol_24 = lum(cb);
      b = set_lum(tint_symbol_23, tint_symbol_24);
      break;
    }
    case 15u: {
      float3 tint_symbol_25 = cb;
      float tint_symbol_26 = lum(cs);
      b = set_lum(tint_symbol_25, tint_symbol_26);
      break;
    }
    default: {
      b = cs;
      break;
    }
  }
  return b;
}

float4 blend_compose(float3 cb, float3 cs, float ab, float as_, uint compose_mode) {
  float fa = 0.0f;
  float fb = 0.0f;
  switch(compose_mode) {
    case 1u: {
      fa = 1.0f;
      fb = 0.0f;
      break;
    }
    case 2u: {
      fa = 0.0f;
      fb = 1.0f;
      break;
    }
    case 3u: {
      fa = 1.0f;
      fb = (1.0f - as_);
      break;
    }
    case 4u: {
      fa = (1.0f - ab);
      fb = 1.0f;
      break;
    }
    case 5u: {
      fa = ab;
      fb = 0.0f;
      break;
    }
    case 6u: {
      fa = 0.0f;
      fb = as_;
      break;
    }
    case 7u: {
      fa = (1.0f - ab);
      fb = 0.0f;
      break;
    }
    case 8u: {
      fa = 0.0f;
      fb = (1.0f - as_);
      break;
    }
    case 9u: {
      fa = ab;
      fb = (1.0f - as_);
      break;
    }
    case 10u: {
      fa = (1.0f - ab);
      fb = as_;
      break;
    }
    case 11u: {
      fa = (1.0f - ab);
      fb = (1.0f - as_);
      break;
    }
    case 12u: {
      fa = 1.0f;
      fb = 1.0f;
      break;
    }
    case 13u: {
      return min((1.0f).xxxx, float4(((as_ * cs) + (ab * cb)), (as_ + ab)));
      break;
    }
    default: {
      break;
    }
  }
  float as_fa = (as_ * fa);
  float ab_fb = (ab * fb);
  float3 co = ((as_fa * cs) + (ab_fb * cb));
  return float4(co, min((as_fa + ab_fb), 1.0f));
}

float3 unpremultiply(float4 color) {
  float EPSILON = 0.000000000000001f;
  float inv_alpha = (1.0f / max(color.a, EPSILON));
  return (color.rgb * inv_alpha);
}

float4 blend_mix_compose(float4 backdrop, float4 src, uint mode) {
  if (((mode & 32767u) == 3u)) {
    return ((backdrop * (1.0f - src.a)) + src);
  }
  float3 cs = unpremultiply(src);
  float3 cb = unpremultiply(backdrop);
  uint mix_mode = (mode >> 8u);
  float3 mixed = blend_mix(cb, cs, mix_mode);
  cs = lerp(cs, mixed, backdrop.a);
  uint compose_mode = (mode & 255u);
  if ((compose_mode == 3u)) {
    float3 co = lerp(backdrop.rgb, cs, src.a);
    return float4(co, (src.a + (backdrop.a * (1.0f - src.a))));
  } else {
    return blend_compose(cb, cs, backdrop.a, src.a, compose_mode);
  }
}

cbuffer cbuffer_constants : register(b1) {
  uint4 constants[11];
};
cbuffer cbuffer_perFrame : register(b2) {
  uint4 perFrame[3];
};
SamplerState boundTexture_s : register(s6);
Texture2D<float4> gradTex_t : register(t8);
Texture2D<float4> fontTex_t : register(t9);
Texture2D<float4> boundTexture_t : register(t10);
Texture2D<float4> backTexture_t : register(t11);

uint constant_shader() {
  return uint((constants[1].x & 255u));
}

bool constant_has_texture() {
  return (((constants[1].x >> 8u) & 255u) != 0u);
}

uint constant_gradient_type() {
  return uint(((constants[1].x >> 16u) & 255u));
}

uint constant_subpixel_mode() {
  return uint(((constants[1].x >> 24u) & 255u));
}

uint constant_blur_directions() {
  return (constants[1].y & 255u);
}

int constant_texture_channel() {
  return int(((constants[1].y >> 8u) & 255u));
}

int constant_sampler_mode() {
  return int(((constants[1].y >> 16u) & 255u));
}

int constant_sprite_oversampling() {
  return int(((constants[1].y >> 24u) & 255u));
}

uint constant_composition_mode() {
  return (constants[1].z & 65535u);
}

bool constant_has_backdrop() {
  return (((constants[1].z >> 16u) & 255u) != 0u);
}

float2 map(float2 p1, float2 p2) {
  return float2(((p1.x * p2.x) + (p1.y * p2.y)), ((p1.x * p2.y) - (p1.y * p2.x)));
}

float4 simpleGradient(float pos) {
  return lerp(asfloat(constants[8]), asfloat(constants[9]), pos);
}

float4 multiGradient(float pos) {
  if ((pos <= 0.0f)) {
    return gradTex_t.Load(uint3(6u, uint(asint(constants[2].x)), 0u));
  }
  if ((pos >= 1.0f)) {
    return gradTex_t.Load(uint3(29u, uint(asint(constants[2].x)), 0u));
  }
  float prev = 0.0f;
  {
    for(uint block = 0u; (block < 6u); block = (block + 1u)) {
      float4 stops4 = gradTex_t.Load(uint3(block, uint(asint(constants[2].x)), 0u));
      {
        for(uint j = 0u; (j < 4u); j = (j + 1u)) {
          uint index = ((block * 4u) + j);
          float curr = stops4[j];
          bool tint_tmp = (pos >= prev);
          if (tint_tmp) {
            tint_tmp = (pos < curr);
          }
          if ((tint_tmp)) {
            float4 c0 = gradTex_t.Load(uint3((6u + max((index - 1u), 0u)), uint(asint(constants[2].x)), 0u));
            float4 c1 = gradTex_t.Load(uint3((6u + index), uint(asint(constants[2].x)), 0u));
            return lerp(c0, c1, ((pos - prev) / ((curr - prev) + 0.00000000099999997172f)));
          }
          prev = curr;
        }
      }
    }
  }
  return (0.0f).xxxx;
}

float3x2 constants_load_3(uint offset) {
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
  float2 transformed_uv = mul(float3(uv, 1.0f), constants_load_3(72u)).xy;
  int tint_symbol_27 = constant_sampler_mode();
  if ((tint_symbol_27 == 0)) {
    transformed_uv = clamp(transformed_uv, (0.5f).xx, (tex_size - 0.5f));
  }
  return (transformed_uv / tex_size);
}

float2 transformedBackTexCoord(float2 uv) {
  uint2 tint_tmp_2;
  backTexture_t.GetDimensions(tint_tmp_2.x, tint_tmp_2.y);
  float2 tex_size = float2(tint_tmp_2);
  float2 transformed_uv = mul(float3(uv, 1.0f), constants_load_3(96u)).xy;
  int tint_symbol_28 = constant_sampler_mode();
  if ((tint_symbol_28 == 0)) {
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
  uint2 tint_tmp_3;
  boundTexture_t.GetDimensions(tint_tmp_3.x, tint_tmp_3.y);
  int2 texSize = int2(tint_tmp_3);
  float sigma = asfloat(constants[2].y);
  int half_size = tint_ftoi(ceil((sigma * 3.0f)));
  float g1 = (1.0f / ((6.283184051513671875f * sigma) * sigma));
  float g2 = (-0.5f / (sigma * sigma));
  float2 w = (1.0f / float2(texSize));
  float2 lo = ((0.5f).xx / float2(texSize));
  float2 hi = (float2((float2(texSize) - 0.5f)) / float2(texSize));
  float tint_symbol_29 = g1;
  float4 tint_symbol_30 = boundTexture_t.Sample(boundTexture_s, pos);
  float4 sum = (tint_symbol_29 * tint_symbol_30);
  {
    for(int i = 1; (i <= half_size); i = (i + 2)) {
      {
        for(int j = 0; (j <= half_size); j = (j + 2)) {
          float tint_symbol_31 = g1;
          float4 tint_symbol_32 = sqr4(float2(float(i), float((i + 1))).xyxy);
          float4 tint_symbol_33 = sqr4(float2(float(j), float((j + 1))).xxyy);
          float4 tint_symbol_34 = exp(((tint_symbol_32 + tint_symbol_33) * g2));
          float4 abcd = (tint_symbol_31 * tint_symbol_34);
          float ab_sum = (abcd[0] + abcd[1]);
          float cd_sum = (abcd[2] + abcd[3]);
          float abcd_sum = (ab_sum + cd_sum);
          float ab_y = (abcd[1] / ab_sum);
          float cd_y = (abcd[3] / cd_sum);
          float abcd_y = (cd_sum / abcd_sum);
          float2 o = float2((((1.0f - abcd_y) * ab_y) + (abcd_y * cd_y)), abcd_y);
          float2 xy = (float2(float(i), float(j)) + o);
          int tint_symbol_35 = constant_sampler_mode();
          if ((tint_symbol_35 == 0)) {
            float4 v1 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * xy)), lo, hi));
            float2 tint_symbol_36 = pos;
            float2 tint_symbol_37 = w;
            float2 tint_symbol_38 = conj(xy);
            float2 tint_symbol_39 = clamp((tint_symbol_36 + (tint_symbol_37 * tint_symbol_38.yx)), lo, hi);
            float4 v2 = boundTexture_t.Sample(boundTexture_s, tint_symbol_39);
            float4 v3 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * -(xy))), lo, hi));
            float2 tint_symbol_40 = pos;
            float2 tint_symbol_41 = w;
            float2 tint_symbol_42 = conj(xy.yx);
            float2 tint_symbol_43 = clamp((tint_symbol_40 + (tint_symbol_41 * tint_symbol_42)), lo, hi);
            float4 v4 = boundTexture_t.Sample(boundTexture_s, tint_symbol_43);
            sum = (sum + ((((v1 + v2) + v3) + v4) * abcd_sum));
          } else {
            float4 v1 = boundTexture_t.Sample(boundTexture_s, (pos + (w * xy)));
            float2 tint_symbol_44 = pos;
            float2 tint_symbol_45 = w;
            float2 tint_symbol_46 = conj(xy);
            float4 v2 = boundTexture_t.Sample(boundTexture_s, (tint_symbol_44 + (tint_symbol_45 * tint_symbol_46.yx)));
            float4 v3 = boundTexture_t.Sample(boundTexture_s, (pos + (w * -(xy))));
            float2 tint_symbol_47 = pos;
            float2 tint_symbol_48 = w;
            float2 tint_symbol_49 = conj(xy.yx);
            float4 v4 = boundTexture_t.Sample(boundTexture_s, (tint_symbol_47 + (tint_symbol_48 * tint_symbol_49)));
            sum = (sum + ((((v1 + v2) + v3) + v4) * abcd_sum));
          }
        }
      }
    }
  }
  return sum;
}

float4 sampleBlur_1d(float2 pos, float2 direction) {
  uint2 tint_tmp_4;
  boundTexture_t.GetDimensions(tint_tmp_4.x, tint_tmp_4.y);
  int2 texSize = int2(tint_tmp_4);
  float sigma = asfloat(constants[2].y);
  int half_size = tint_ftoi(ceil((sigma * 3.0f)));
  float g1 = (1.0f / (2.5066280364990234375f * sigma));
  float g2 = (-0.5f / (sigma * sigma));
  float2 w = (1.0f / float2(texSize));
  float2 lo = ((0.5f).xx / float2(texSize));
  float2 hi = (float2((float2(texSize) - 0.5f)) / float2(texSize));
  float tint_symbol_50 = g1;
  float4 tint_symbol_51 = boundTexture_t.Sample(boundTexture_s, pos);
  float4 sum = (tint_symbol_50 * tint_symbol_51);
  {
    for(int i = 1; (i <= half_size); i = (i + 2)) {
      float tint_symbol_52 = g1;
      float2 tint_symbol_53 = sqr2(float2(float(i), float((i + 1))));
      float2 tint_symbol_54 = exp((tint_symbol_53 * g2));
      float2 weights = (tint_symbol_52 * tint_symbol_54);
      float weight_sum = (weights[0] + weights[1]);
      float offset = (weights[1] / weight_sum);
      float2 sample_offset = ((float(i) + offset) * direction);
      int tint_symbol_55 = constant_sampler_mode();
      if ((tint_symbol_55 == 0)) {
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
  uint tint_symbol_56 = constant_blur_directions();
  if ((tint_symbol_56 == 1u)) {
    return sampleBlur_1d(pos, float2(1.0f, 0.0f));
  } else {
    uint tint_symbol_57 = constant_blur_directions();
    if ((tint_symbol_57 == 2u)) {
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
  uint tint_symbol_61 = constant_gradient_type();
  if ((tint_symbol_61 == 0u)) {
    return positionAlongLine(asfloat(constants[10].xy), asfloat(constants[10].zw), tint_symbol);
  } else {
    uint tint_symbol_62 = constant_gradient_type();
    if ((tint_symbol_62 == 1u)) {
      return (length((tint_symbol - asfloat(constants[10].xy))) / length((asfloat(constants[10].zw) - asfloat(constants[10].xy))));
    } else {
      uint tint_symbol_63 = constant_gradient_type();
      if ((tint_symbol_63 == 2u)) {
        return angleGradient(tint_symbol, asfloat(constants[10].xy), asfloat(constants[10].zw));
      } else {
        uint tint_symbol_64 = constant_gradient_type();
        if ((tint_symbol_64 == 3u)) {
          float tint_symbol_65 = positionAlongLine(asfloat(constants[10].xy), asfloat(constants[10].zw), tint_symbol);
          float tint_symbol_66 = frac(tint_symbol_65);
          float tint_symbol_67 = abs(((tint_symbol_66 * 2.0f) - 1.0f));
          return (1.0f - tint_symbol_67);
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
  if (!(constant_has_texture())) {
    float grad_pos = gradientPosition(canvas_coord);
    if ((asint(constants[2].x) == -1)) {
      result = simpleGradient(grad_pos);
    } else {
      result = multiGradient(grad_pos);
    }
    return result;
  }
  float2 transformed_uv = transformedTexCoord(canvas_coord);
  bool tint_symbol_58 = (asfloat(constants[2].y) > 0.0f);
  if (tint_symbol_58) {
    uint tint_symbol_59 = constant_blur_directions();
    tint_symbol_58 = (tint_symbol_59 != 0u);
  }
  if (tint_symbol_58) {
    result = sampleBlur(transformed_uv);
  } else {
    result = boundTexture_t.Sample(boundTexture_s, transformed_uv);
  }
  result = clamp(result, (0.0f).xxxx, (1.0f).xxxx);
  if ((asint(constants[2].x) != -1)) {
    int tint_symbol_60 = constant_texture_channel();
    result = multiGradient(result[tint_symbol_60]);
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
  int tint_symbol_68 = constant_sprite_oversampling();
  if ((tint_symbol_68 == 1)) {
    alpha = atlas(sprite, pos, stride);
    return alpha;
  }
  {
    int i = 0;
    while (true) {
      int tint_symbol_72 = i;
      int tint_symbol_73 = constant_sprite_oversampling();
      if (!((tint_symbol_72 < tint_symbol_73))) {
        break;
      }
      {
        float tint_symbol_74 = alpha;
        float tint_symbol_75 = atlas(sprite, (pos + int2(i, 0)), stride);
        alpha = (tint_symbol_74 + tint_symbol_75);
      }
      {
        i = (i + 1);
      }
    }
  }
  float tint_symbol_69 = alpha;
  int tint_symbol_70 = constant_sprite_oversampling();
  float tint_symbol_71 = float(tint_symbol_70);
  return (tint_symbol_69 / tint_symbol_71);
}

float3 processSubpixelOutput(float3 rgb) {
  uint tint_symbol_76 = constant_subpixel_mode();
  if ((tint_symbol_76 == 0u)) {
    return float3((dot(rgb, float3(0.33329999446868896484f, 0.33340001106262207031f, 0.33329999446868896484f))).xxx);
  } else {
    uint tint_symbol_77 = constant_subpixel_mode();
    if ((tint_symbol_77 == 2u)) {
      return rgb.bgr;
    } else {
      return rgb;
    }
  }
}

float3 atlasSubpixel(int sprite, int2 pos, uint stride) {
  int tint_symbol_78 = constant_sprite_oversampling();
  if ((tint_symbol_78 == 6)) {
    float tint_symbol_79 = atlas(sprite, (pos + int2(-2, 0)), stride);
    float tint_symbol_80 = atlas(sprite, (pos + int2(-1, 0)), stride);
    float x0 = (tint_symbol_79 + tint_symbol_80);
    float tint_symbol_81 = atlas(sprite, (pos + (0).xx), stride);
    float tint_symbol_82 = atlas(sprite, (pos + int2(1, 0)), stride);
    float x1 = (tint_symbol_81 + tint_symbol_82);
    float tint_symbol_83 = atlas(sprite, (pos + int2(2, 0)), stride);
    float tint_symbol_84 = atlas(sprite, (pos + int2(3, 0)), stride);
    float x2 = (tint_symbol_83 + tint_symbol_84);
    float tint_symbol_85 = atlas(sprite, (pos + int2(4, 0)), stride);
    float tint_symbol_86 = atlas(sprite, (pos + int2(5, 0)), stride);
    float x3 = (tint_symbol_85 + tint_symbol_86);
    float tint_symbol_87 = atlas(sprite, (pos + int2(6, 0)), stride);
    float tint_symbol_88 = atlas(sprite, (pos + int2(7, 0)), stride);
    float x4 = (tint_symbol_87 + tint_symbol_88);
    float3 filt = float3(0.125f, 0.25f, 0.125f);
    return float3(dot(float3(x0, x1, x2), filt), dot(float3(x1, x2, x3), filt), dot(float3(x2, x3, x4), filt));
  } else {
    int tint_symbol_89 = constant_sprite_oversampling();
    if ((tint_symbol_89 == 3)) {
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
  float2 tint_symbol_90 = erf(((float2((x).xx) + float2(-(curved), curved)) * (0.70710676908493041992f / sigma)));
  float2 integral = (0.5f + (0.5f * tint_symbol_90));
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
      float tint_symbol_91 = value;
      float tint_symbol_92 = roundedBoxShadowX(tint_symbol.x, (tint_symbol.y - y), sigma, corner, halfSize);
      float tint_symbol_93 = gaussian(y, sigma);
      value = (tint_symbol_91 + ((tint_symbol_92 * tint_symbol_93) * step));
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
  uint tint_symbol_96 = constant_shader();
  bool tint_symbol_95 = (tint_symbol_96 == 1u);
  if (!(tint_symbol_95)) {
    uint tint_symbol_97 = constant_shader();
    tint_symbol_95 = (tint_symbol_97 == 5u);
  }
  bool tint_symbol_94 = tint_symbol_95;
  if (tint_symbol_94) {
    uint tint_symbol_98 = constant_subpixel_mode();
    tint_symbol_94 = (tint_symbol_98 != 0u);
  }
  return tint_symbol_94;
}

FragOut postprocessColor(FragOut tint_symbol_2, float2 canvas_coord) {
  FragOut tint_symbol_3 = tint_symbol_2;
  float opacity = asfloat(constants[7].w);
  if ((constants[7].z != 0u)) {
    uint pattern_scale = (constants[7].z >> 24u);
    uint hpattern = (constants[7].z & 4095u);
    uint vpattern = (constants[7].z >> 12u);
    uint2 coords = tint_ftou(canvas_coord);
    uint tint_symbol_103 = samplePattern(tint_div(coords.x, pattern_scale), hpattern);
    uint tint_symbol_104 = samplePattern(tint_div(coords.y, pattern_scale), vpattern);
    uint p = (tint_symbol_103 & tint_symbol_104);
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
  if (constant_has_backdrop()) {
    float2 tint_symbol_99 = transformedBackTexCoord(canvas_coord);
    float4 tint_symbol_4 = backTexture_t.Sample(boundTexture_s, tint_symbol_99);
    float4 tint_symbol_100 = tint_symbol_4;
    float4 tint_symbol_101 = tint_symbol_3.color;
    uint tint_symbol_102 = constant_composition_mode();
    tint_symbol_3.color = blend_mix_compose(tint_symbol_100, tint_symbol_101, tint_symbol_102);
    tint_symbol_3.blend = float4((tint_symbol_3.color.a).xxxx);
  } else {
    if (!(useBlending())) {
      tint_symbol_3.blend = float4((tint_symbol_3.color.a).xxxx);
    }
  }
  return tint_symbol_3;
}

float rectangleCoverage(float2 pt, float4 rect) {
  float2 wh = max((0.0f).xx, (min((pt.xy + (0.5f).xx), rect.zw) - max((pt.xy - (0.5f).xx), rect.xy)));
  return (wh.x * wh.y);
}

struct tint_symbol_118 {
  noperspective float4 data0 : TEXCOORD0;
  noperspective float4 data1 : TEXCOORD1;
  noperspective float2 uv : TEXCOORD2;
  noperspective float2 canvas_coord : TEXCOORD3;
  nointerpolation uint4 coverage : TEXCOORD4;
  float4 position : SV_Position;
};
struct tint_symbol_119 {
  float4 color : SV_Target0;
  float4 blend : SV_Target1;
};

FragOut fragmentMain_inner(VertexOutput tint_symbol_2) {
  uint tint_symbol_105 = constant_shader();
  if ((tint_symbol_105 == 4u)) {
    int2 tex_coord = tint_ftoi_1(tint_symbol_2.position.xy);
    FragOut tint_symbol_120 = {boundTexture_t.Load(int3(tex_coord, 0)), (1.0f).xxxx};
    return tint_symbol_120;
  }
  float4 outColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
  float4 outBlend = float4(0.0f, 0.0f, 0.0f, 0.0f);
  uint tint_symbol_106 = constant_shader();
  if ((tint_symbol_106 == 2u)) {
    float tint_symbol_107 = roundedBoxShadow((tint_symbol_2.data0.xy * 0.5f), tint_symbol_2.uv, asfloat(constants[2].y), tint_symbol_2.data1);
    outColor = (asfloat(constants[8]) * tint_symbol_107);
  } else {
    uint tint_symbol_109 = constant_shader();
    bool tint_symbol_108 = (tint_symbol_109 == 3u);
    if (!(tint_symbol_108)) {
      uint tint_symbol_110 = constant_shader();
      tint_symbol_108 = (tint_symbol_110 == 1u);
    }
    if (tint_symbol_108) {
      int sprite = tint_ftoi(tint_symbol_2.data0.z);
      uint stride = tint_ftou_1(tint_symbol_2.data0.w);
      int2 tuv = tint_ftoi_1(tint_symbol_2.uv);
      float4 shadeColor = computeShadeColor(tint_symbol_2.canvas_coord);
      if (useBlending()) {
        float3 tint_symbol_111 = atlasSubpixel(sprite, tuv, stride);
        float3 rgb = processSubpixelOutput(tint_symbol_111);
        outColor = (shadeColor * float4(rgb, 1.0f));
        outBlend = float4((shadeColor.a * rgb), 1.0f);
      } else {
        uint tint_symbol_112 = constant_shader();
        if ((tint_symbol_112 == 3u)) {
          float4 tint_symbol_113 = shadeColor;
          float4 tint_symbol_114 = atlasRGBA(sprite, tuv, stride);
          outColor = (tint_symbol_113 * tint_symbol_114);
        } else {
          float alpha = atlasAccum(sprite, tuv, stride);
          outColor = (shadeColor * float4((alpha).xxxx));
        }
      }
    } else {
      uint tint_symbol_115 = constant_shader();
      if ((tint_symbol_115 == 5u)) {
        uint2 xy = tint_ftou(tint_symbol_2.uv);
        float cov = tint_unpack4x8unorm(tint_symbol_2.coverage[(xy.y & 3u)])[(xy.x & 3u)];
        float4 shadeColor = computeShadeColor(tint_symbol_2.canvas_coord);
        outColor = (shadeColor * float4((cov).xxxx));
      } else {
        uint tint_symbol_116 = constant_shader();
        if ((tint_symbol_116 == 0u)) {
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
  FragOut tint_symbol_121 = {outColor, outBlend};
  return postprocessColor(tint_symbol_121, tint_symbol_2.canvas_coord);
}

tint_symbol_119 fragmentMain(tint_symbol_118 tint_symbol_117) {
  VertexOutput tint_symbol_122 = {float4(tint_symbol_117.position.xyz, (1.0f / tint_symbol_117.position.w)), tint_symbol_117.data0, tint_symbol_117.data1, tint_symbol_117.uv, tint_symbol_117.canvas_coord, tint_symbol_117.coverage};
  FragOut inner_result = fragmentMain_inner(tint_symbol_122);
  tint_symbol_119 wrapper_result = (tint_symbol_119)0;
  wrapper_result.color = inner_result.color;
  wrapper_result.blend = inner_result.blend;
  return wrapper_result;
}
