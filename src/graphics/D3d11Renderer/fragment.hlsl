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
  float3 tint_symbol_6 = screen(cb, ((2.0f * cs) - 1.0f));
  return ((cs <= (0.5f).xxx) ? ((cb * 2.0f) * cs) : tint_symbol_6);
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
  float3 tint_symbol_7 = c;
  float tint_symbol_8 = l;
  float tint_symbol_9 = lum(c);
  return clip_color((tint_symbol_7 + (tint_symbol_8 - tint_symbol_9)));
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
      float tint_symbol_10 = color_dodge(cb.x, cs.x);
      float tint_symbol_11 = color_dodge(cb.y, cs.y);
      float tint_symbol_12 = color_dodge(cb.z, cs.z);
      b = float3(tint_symbol_10, tint_symbol_11, tint_symbol_12);
      break;
    }
    case 7u: {
      float tint_symbol_13 = color_burn(cb.x, cs.x);
      float tint_symbol_14 = color_burn(cb.y, cs.y);
      float tint_symbol_15 = color_burn(cb.z, cs.z);
      b = float3(tint_symbol_13, tint_symbol_14, tint_symbol_15);
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
      float3 tint_symbol_16 = cs;
      float tint_symbol_17 = sat(cb);
      float3 tint_symbol_18 = set_sat(tint_symbol_16, tint_symbol_17);
      float tint_symbol_19 = lum(cb);
      b = set_lum(tint_symbol_18, tint_symbol_19);
      break;
    }
    case 13u: {
      float3 tint_symbol_20 = cb;
      float tint_symbol_21 = sat(cs);
      float3 tint_symbol_22 = set_sat(tint_symbol_20, tint_symbol_21);
      float tint_symbol_23 = lum(cb);
      b = set_lum(tint_symbol_22, tint_symbol_23);
      break;
    }
    case 14u: {
      float3 tint_symbol_24 = cs;
      float tint_symbol_25 = lum(cb);
      b = set_lum(tint_symbol_24, tint_symbol_25);
      break;
    }
    case 15u: {
      float3 tint_symbol_26 = cb;
      float tint_symbol_27 = lum(cs);
      b = set_lum(tint_symbol_26, tint_symbol_27);
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
  int tint_symbol_28 = constant_sampler_mode();
  if ((tint_symbol_28 == 0)) {
    transformed_uv = clamp(transformed_uv, (0.5f).xx, (tex_size - 0.5f));
  }
  return (transformed_uv / tex_size);
}

float2 transformedBackTexCoord(float2 uv) {
  uint2 tint_tmp_2;
  backTexture_t.GetDimensions(tint_tmp_2.x, tint_tmp_2.y);
  float2 tex_size = float2(tint_tmp_2);
  float2 transformed_uv = mul(float3(uv, 1.0f), constants_load_3(96u)).xy;
  int tint_symbol_29 = constant_sampler_mode();
  if ((tint_symbol_29 == 0)) {
    transformed_uv = clamp(transformed_uv, (0.5f).xx, (tex_size - 0.5f));
  }
  return (transformed_uv / tex_size);
}

float sqr(float x) {
  return (x * x);
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

float4 gaussianBlur_2d(float2 pos) {
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
  float tint_symbol_30 = g1;
  float4 tint_symbol_31 = boundTexture_t.Sample(boundTexture_s, pos);
  float4 sum = (tint_symbol_30 * tint_symbol_31);
  {
    for(int i = 1; (i <= half_size); i = (i + 2)) {
      {
        for(int j = 0; (j <= half_size); j = (j + 2)) {
          float tint_symbol_32 = g1;
          float4 tint_symbol_33 = sqr4(float2(float(i), float((i + 1))).xyxy);
          float4 tint_symbol_34 = sqr4(float2(float(j), float((j + 1))).xxyy);
          float4 tint_symbol_35 = exp(((tint_symbol_33 + tint_symbol_34) * g2));
          float4 abcd = (tint_symbol_32 * tint_symbol_35);
          float ab_sum = (abcd[0] + abcd[1]);
          float cd_sum = (abcd[2] + abcd[3]);
          float abcd_sum = (ab_sum + cd_sum);
          float ab_y = (abcd[1] / ab_sum);
          float cd_y = (abcd[3] / cd_sum);
          float abcd_y = (cd_sum / abcd_sum);
          float2 o = float2((((1.0f - abcd_y) * ab_y) + (abcd_y * cd_y)), abcd_y);
          float2 xy = (float2(float(i), float(j)) + o);
          int tint_symbol_36 = constant_sampler_mode();
          if ((tint_symbol_36 == 0)) {
            float4 v1 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * xy)), lo, hi));
            float2 tint_symbol_37 = pos;
            float2 tint_symbol_38 = w;
            float2 tint_symbol_39 = conj(xy);
            float2 tint_symbol_40 = clamp((tint_symbol_37 + (tint_symbol_38 * tint_symbol_39.yx)), lo, hi);
            float4 v2 = boundTexture_t.Sample(boundTexture_s, tint_symbol_40);
            float4 v3 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * -(xy))), lo, hi));
            float2 tint_symbol_41 = pos;
            float2 tint_symbol_42 = w;
            float2 tint_symbol_43 = conj(xy.yx);
            float2 tint_symbol_44 = clamp((tint_symbol_41 + (tint_symbol_42 * tint_symbol_43)), lo, hi);
            float4 v4 = boundTexture_t.Sample(boundTexture_s, tint_symbol_44);
            sum = (sum + ((((v1 + v2) + v3) + v4) * abcd_sum));
          } else {
            float4 v1 = boundTexture_t.Sample(boundTexture_s, (pos + (w * xy)));
            float2 tint_symbol_45 = pos;
            float2 tint_symbol_46 = w;
            float2 tint_symbol_47 = conj(xy);
            float4 v2 = boundTexture_t.Sample(boundTexture_s, (tint_symbol_45 + (tint_symbol_46 * tint_symbol_47.yx)));
            float4 v3 = boundTexture_t.Sample(boundTexture_s, (pos + (w * -(xy))));
            float2 tint_symbol_48 = pos;
            float2 tint_symbol_49 = w;
            float2 tint_symbol_50 = conj(xy.yx);
            float4 v4 = boundTexture_t.Sample(boundTexture_s, (tint_symbol_48 + (tint_symbol_49 * tint_symbol_50)));
            sum = (sum + ((((v1 + v2) + v3) + v4) * abcd_sum));
          }
        }
      }
    }
  }
  return sum;
}

float4 gaussianBlur_1d(float2 pos, float2 direction) {
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
  float tint_symbol_51 = g1;
  float4 tint_symbol_52 = boundTexture_t.Sample(boundTexture_s, pos);
  float4 sum = (tint_symbol_51 * tint_symbol_52);
  {
    for(int i = 1; (i <= half_size); i = (i + 2)) {
      float tint_symbol_53 = g1;
      float2 tint_symbol_54 = sqr2(float2(float(i), float((i + 1))));
      float2 tint_symbol_55 = exp((tint_symbol_54 * g2));
      float2 weights = (tint_symbol_53 * tint_symbol_55);
      float weight_sum = (weights[0] + weights[1]);
      float offset = (weights[1] / weight_sum);
      float2 sample_offset = ((float(i) + offset) * direction);
      int tint_symbol_56 = constant_sampler_mode();
      if ((tint_symbol_56 == 0)) {
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

float4 boxBlur_2d(float2 pos) {
  uint2 tint_tmp_5;
  boundTexture_t.GetDimensions(tint_tmp_5.x, tint_tmp_5.y);
  int2 texSize = int2(tint_tmp_5);
  float radius = asfloat(constants[2].y);
  float2 w = (1.0f / float2(texSize));
  float2 lo = ((0.5f).xx / float2(texSize));
  float2 hi = (float2((float2(texSize) - 0.5f)) / float2(texSize));
  float4 sum = (0.0f).xxxx;
  int halfside = tint_ftoi(ceil(radius));
  {
    for(int i = -(halfside); (i <= halfside); i = (i + 1)) {
      {
        for(int j = -(halfside); (j <= halfside); j = (j + 1)) {
          float fx = (1.0f - max(0.0f, (abs(float(i)) - radius)));
          float fy = (1.0f - max(0.0f, (abs(float(j)) - radius)));
          float weight = (fx * fy);
          float2 offset = float2(float(i), float(j));
          float4 tint_symbol = float4(0.0f, 0.0f, 0.0f, 0.0f);
          int tint_symbol_59 = constant_sampler_mode();
          if ((tint_symbol_59 == 0)) {
            tint_symbol = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * offset)), lo, hi));
          } else {
            tint_symbol = boundTexture_t.Sample(boundTexture_s, (pos + (w * offset)));
          }
          sum = (sum + (tint_symbol * weight));
        }
      }
    }
  }
  float4 tint_symbol_57 = sum;
  float tint_symbol_58 = sqr(((radius * 2.0f) + 1.0f));
  return (tint_symbol_57 / tint_symbol_58);
}

float4 boxBlur_1d(float2 pos, float2 direction) {
  uint2 tint_tmp_6;
  boundTexture_t.GetDimensions(tint_tmp_6.x, tint_tmp_6.y);
  int2 texSize = int2(tint_tmp_6);
  float radius = asfloat(constants[2].y);
  float2 w = (1.0f / float2(texSize));
  float2 lo = ((0.5f).xx / float2(texSize));
  float2 hi = (float2((float2(texSize) - 0.5f)) / float2(texSize));
  float4 sum = boundTexture_t.Sample(boundTexture_s, pos);
  int halfside = tint_ftoi(ceil(radius));
  {
    for(int i = 1; (i <= halfside); i = (i + 1)) {
      float weight = (1.0f - max(0.0f, (abs(float(i)) - radius)));
      float2 sample_offset = (float(i) * direction);
      int tint_symbol_60 = constant_sampler_mode();
      if ((tint_symbol_60 == 0)) {
        float4 v1 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * sample_offset)), lo, hi));
        float4 v2 = boundTexture_t.Sample(boundTexture_s, clamp((pos - (w * sample_offset)), lo, hi));
        sum = (sum + (weight * (v1 + v2)));
      } else {
        float4 v1 = boundTexture_t.Sample(boundTexture_s, (pos + (w * sample_offset)));
        float4 v2 = boundTexture_t.Sample(boundTexture_s, (pos - (w * sample_offset)));
        sum = (sum + (weight * (v1 + v2)));
      }
    }
  }
  return (sum / ((radius * 2.0f) + 1.0f));
}

float4 sampleBlur(float2 pos) {
  uint tint_symbol_61 = constant_blur_directions();
  bool box = ((tint_symbol_61 & 4u) != 0u);
  uint tint_symbol_62 = constant_blur_directions();
  uint dir = (tint_symbol_62 & 3u);
  if (box) {
    if ((dir == 1u)) {
      return boxBlur_1d(pos, float2(1.0f, 0.0f));
    } else {
      if ((dir == 2u)) {
        return boxBlur_1d(pos, float2(0.0f, 1.0f));
      } else {
        return boxBlur_2d(pos);
      }
    }
  } else {
    if ((dir == 1u)) {
      return gaussianBlur_1d(pos, float2(1.0f, 0.0f));
    } else {
      if ((dir == 2u)) {
        return gaussianBlur_1d(pos, float2(0.0f, 1.0f));
      } else {
        return gaussianBlur_2d(pos);
      }
    }
  }
}

float positionAlongLine(float2 from_, float2 to, float2 tint_symbol_1) {
  float2 dir = normalize((to - from_));
  float2 offs = (tint_symbol_1 - from_);
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

float gradientPositionForPoint(float2 tint_symbol_1) {
  uint tint_symbol_66 = constant_gradient_type();
  if ((tint_symbol_66 == 0u)) {
    return positionAlongLine(asfloat(constants[10].xy), asfloat(constants[10].zw), tint_symbol_1);
  } else {
    uint tint_symbol_67 = constant_gradient_type();
    if ((tint_symbol_67 == 1u)) {
      return (length((tint_symbol_1 - asfloat(constants[10].xy))) / length((asfloat(constants[10].zw) - asfloat(constants[10].xy))));
    } else {
      uint tint_symbol_68 = constant_gradient_type();
      if ((tint_symbol_68 == 2u)) {
        return angleGradient(tint_symbol_1, asfloat(constants[10].xy), asfloat(constants[10].zw));
      } else {
        uint tint_symbol_69 = constant_gradient_type();
        if ((tint_symbol_69 == 3u)) {
          float tint_symbol_70 = positionAlongLine(asfloat(constants[10].xy), asfloat(constants[10].zw), tint_symbol_1);
          float tint_symbol_71 = frac(tint_symbol_70);
          float tint_symbol_72 = abs(((tint_symbol_71 * 2.0f) - 1.0f));
          return (1.0f - tint_symbol_72);
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
  bool tint_symbol_63 = (asfloat(constants[2].y) > 0.0f);
  if (tint_symbol_63) {
    uint tint_symbol_64 = constant_blur_directions();
    tint_symbol_63 = (tint_symbol_64 != 0u);
  }
  if (tint_symbol_63) {
    result = sampleBlur(transformed_uv);
  } else {
    result = boundTexture_t.Sample(boundTexture_s, transformed_uv);
  }
  result = clamp(result, (0.0f).xxxx, (1.0f).xxxx);
  if ((asint(constants[2].x) != -1)) {
    int tint_symbol_65 = constant_texture_channel();
    result = multiGradient(result[tint_symbol_65]);
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
  bool tint_tmp_7 = (pos.x < 0);
  if (!tint_tmp_7) {
    tint_tmp_7 = (pos.x >= int(stride));
  }
  if ((tint_tmp_7)) {
    return 0.0f;
  }
  if ((sprite < 0)) {
    return float((uint((pos.x & pos.y)) & 1u));
  }
  uint tint_symbol_2 = (((uint(sprite) * 8u) + uint(pos.x)) + (uint(pos.y) * stride));
  return fontTex_t.Load(uint3(tint_mod(tint_symbol_2, perFrame[2].x), tint_div(tint_symbol_2, perFrame[2].x), 0u)).r;
}

float4 atlasRGBA(int sprite, int2 pos, uint stride) {
  bool tint_tmp_8 = (pos.x < 0);
  if (!tint_tmp_8) {
    tint_tmp_8 = (((pos.x * 4) + 3) >= int(stride));
  }
  if ((tint_tmp_8)) {
    return (0.0f).xxxx;
  }
  if ((sprite < 0)) {
    return float4((float((uint((pos.x & pos.y)) & 1u))).xxxx);
  }
  uint tint_symbol_2 = (((uint(sprite) * 8u) + (uint(pos.x) * 4u)) + (uint(pos.y) * stride));
  uint2 c = uint2(tint_mod(tint_symbol_2, perFrame[2].x), tint_div(tint_symbol_2, perFrame[2].x));
  return float4(fontTex_t.Load(uint3(c, 0u)).r, fontTex_t.Load(uint3((c + uint2(1u, 0u)), 0u)).r, fontTex_t.Load(uint3((c + uint2(2u, 0u)), 0u)).r, fontTex_t.Load(uint3((c + uint2(3u, 0u)), 0u)).r);
}

float atlasAccum(int sprite, int2 pos, uint stride) {
  float alpha = 0.0f;
  int tint_symbol_73 = constant_sprite_oversampling();
  if ((tint_symbol_73 == 1)) {
    alpha = atlas(sprite, pos, stride);
    return alpha;
  }
  {
    int i = 0;
    while (true) {
      int tint_symbol_77 = i;
      int tint_symbol_78 = constant_sprite_oversampling();
      if (!((tint_symbol_77 < tint_symbol_78))) {
        break;
      }
      {
        float tint_symbol_79 = alpha;
        float tint_symbol_80 = atlas(sprite, (pos + int2(i, 0)), stride);
        alpha = (tint_symbol_79 + tint_symbol_80);
      }
      {
        i = (i + 1);
      }
    }
  }
  float tint_symbol_74 = alpha;
  int tint_symbol_75 = constant_sprite_oversampling();
  float tint_symbol_76 = float(tint_symbol_75);
  return (tint_symbol_74 / tint_symbol_76);
}

float3 processSubpixelOutput(float3 rgb) {
  uint tint_symbol_81 = constant_subpixel_mode();
  if ((tint_symbol_81 == 0u)) {
    return float3((dot(rgb, float3(0.33329999446868896484f, 0.33340001106262207031f, 0.33329999446868896484f))).xxx);
  } else {
    uint tint_symbol_82 = constant_subpixel_mode();
    if ((tint_symbol_82 == 2u)) {
      return rgb.bgr;
    } else {
      return rgb;
    }
  }
}

float3 atlasSubpixel(int sprite, int2 pos, uint stride) {
  int tint_symbol_83 = constant_sprite_oversampling();
  if ((tint_symbol_83 == 6)) {
    float tint_symbol_84 = atlas(sprite, (pos + int2(-2, 0)), stride);
    float tint_symbol_85 = atlas(sprite, (pos + int2(-1, 0)), stride);
    float x0 = (tint_symbol_84 + tint_symbol_85);
    float tint_symbol_86 = atlas(sprite, (pos + (0).xx), stride);
    float tint_symbol_87 = atlas(sprite, (pos + int2(1, 0)), stride);
    float x1 = (tint_symbol_86 + tint_symbol_87);
    float tint_symbol_88 = atlas(sprite, (pos + int2(2, 0)), stride);
    float tint_symbol_89 = atlas(sprite, (pos + int2(3, 0)), stride);
    float x2 = (tint_symbol_88 + tint_symbol_89);
    float tint_symbol_90 = atlas(sprite, (pos + int2(4, 0)), stride);
    float tint_symbol_91 = atlas(sprite, (pos + int2(5, 0)), stride);
    float x3 = (tint_symbol_90 + tint_symbol_91);
    float tint_symbol_92 = atlas(sprite, (pos + int2(6, 0)), stride);
    float tint_symbol_93 = atlas(sprite, (pos + int2(7, 0)), stride);
    float x4 = (tint_symbol_92 + tint_symbol_93);
    float3 filt = float3(0.125f, 0.25f, 0.125f);
    return float3(dot(float3(x0, x1, x2), filt), dot(float3(x1, x2, x3), filt), dot(float3(x2, x3, x4), filt));
  } else {
    int tint_symbol_94 = constant_sprite_oversampling();
    if ((tint_symbol_94 == 3)) {
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
  float2 tint_symbol_95 = erf(((float2((x).xx) + float2(-(curved), curved)) * (0.70710676908493041992f / sigma)));
  float2 integral = (0.5f + (0.5f * tint_symbol_95));
  return (integral.y - integral.x);
}

float roundedBoxShadow(float2 halfSize, float2 tint_symbol_1, float sigma, float4 border_radii) {
  float low = (tint_symbol_1.y - halfSize.y);
  float high = (tint_symbol_1.y + halfSize.y);
  float start = clamp((-3.0f * sigma), low, high);
  float end = clamp((3.0f * sigma), low, high);
  float step = ((end - start) / 4.0f);
  float y = (start + (step * 0.5f));
  float value = 0.0f;
  uint quadrant = (uint((tint_symbol_1.x >= 0.0f)) + (2u * uint((tint_symbol_1.y >= 0.0f))));
  float corner = abs(border_radii[quadrant]);
  {
    for(int i = 0; (i < 4); i = (i + 1)) {
      float tint_symbol_96 = value;
      float tint_symbol_97 = roundedBoxShadowX(tint_symbol_1.x, (tint_symbol_1.y - y), sigma, corner, halfSize);
      float tint_symbol_98 = gaussian(y, sigma);
      value = (tint_symbol_96 + ((tint_symbol_97 * tint_symbol_98) * step));
      y = (y + step);
    }
  }
  return value;
}

float4 applyGamma(float4 tint_symbol_3, float gamma) {
  return pow(max(tint_symbol_3, (0.0f).xxxx), float4((gamma).xxxx));
}

float4 applyBlueLightFilter(float4 tint_symbol_3, float intensity) {
  return (tint_symbol_3 * float4(1.0f, (1.0f - ((intensity * 0.60000002384185791016f) * 0.60000002384185791016f)), (1.0f - (intensity * 0.60000002384185791016f)), 1.0f));
}

struct FragOut {
  float4 color;
  float4 blend;
};

bool useBlending() {
  uint tint_symbol_101 = constant_shader();
  bool tint_symbol_100 = (tint_symbol_101 == 1u);
  if (!(tint_symbol_100)) {
    uint tint_symbol_102 = constant_shader();
    tint_symbol_100 = (tint_symbol_102 == 5u);
  }
  bool tint_symbol_99 = tint_symbol_100;
  if (tint_symbol_99) {
    uint tint_symbol_103 = constant_subpixel_mode();
    tint_symbol_99 = (tint_symbol_103 != 0u);
  }
  return tint_symbol_99;
}

FragOut postprocessColor(FragOut tint_symbol_3, float2 canvas_coord) {
  FragOut tint_symbol_4 = tint_symbol_3;
  float opacity = asfloat(constants[7].w);
  if ((constants[7].z != 0u)) {
    uint pattern_scale = (constants[7].z >> 24u);
    uint hpattern = (constants[7].z & 4095u);
    uint vpattern = (constants[7].z >> 12u);
    uint2 coords = tint_ftou(canvas_coord);
    uint tint_symbol_108 = samplePattern(tint_div(coords.x, pattern_scale), hpattern);
    uint tint_symbol_109 = samplePattern(tint_div(coords.y, pattern_scale), vpattern);
    uint p = (tint_symbol_108 & tint_symbol_109);
    opacity = (opacity * float(p));
  }
  tint_symbol_4.color = (tint_symbol_4.color * opacity);
  if (useBlending()) {
    tint_symbol_4.blend = (tint_symbol_4.blend * opacity);
  }
  if ((asfloat(perFrame[1].x) != 0.0f)) {
    tint_symbol_4.color = applyBlueLightFilter(tint_symbol_4.color, asfloat(perFrame[1].x));
    if (useBlending()) {
      tint_symbol_4.blend = applyBlueLightFilter(tint_symbol_4.blend, asfloat(perFrame[1].x));
    }
  }
  if ((asfloat(perFrame[1].y) != 1.0f)) {
    tint_symbol_4.color = applyGamma(tint_symbol_4.color, asfloat(perFrame[1].y));
    if (useBlending()) {
      tint_symbol_4.blend = applyGamma(tint_symbol_4.blend, asfloat(perFrame[1].y));
    }
  }
  if (constant_has_backdrop()) {
    float2 tint_symbol_104 = transformedBackTexCoord(canvas_coord);
    float4 tint_symbol_5 = backTexture_t.Sample(boundTexture_s, tint_symbol_104);
    float4 tint_symbol_105 = tint_symbol_5;
    float4 tint_symbol_106 = tint_symbol_4.color;
    uint tint_symbol_107 = constant_composition_mode();
    tint_symbol_4.color = blend_mix_compose(tint_symbol_105, tint_symbol_106, tint_symbol_107);
    tint_symbol_4.blend = float4((tint_symbol_4.color.a).xxxx);
  } else {
    if (!(useBlending())) {
      tint_symbol_4.blend = float4((tint_symbol_4.color.a).xxxx);
    }
  }
  return tint_symbol_4;
}

float rectangleCoverage(float2 pt, float4 rect) {
  float2 wh = max((0.0f).xx, (min((pt.xy + (0.5f).xx), rect.zw) - max((pt.xy - (0.5f).xx), rect.xy)));
  return (wh.x * wh.y);
}

struct tint_symbol_123 {
  noperspective float4 data0 : TEXCOORD0;
  noperspective float4 data1 : TEXCOORD1;
  noperspective float2 uv : TEXCOORD2;
  noperspective float2 canvas_coord : TEXCOORD3;
  nointerpolation uint4 coverage : TEXCOORD4;
  float4 position : SV_Position;
};
struct tint_symbol_124 {
  float4 color : SV_Target0;
  float4 blend : SV_Target1;
};

FragOut fragmentMain_inner(VertexOutput tint_symbol_3) {
  uint tint_symbol_110 = constant_shader();
  if ((tint_symbol_110 == 4u)) {
    int2 tex_coord = tint_ftoi_1(tint_symbol_3.position.xy);
    FragOut tint_symbol_125 = {boundTexture_t.Load(int3(tex_coord, 0)), (1.0f).xxxx};
    return tint_symbol_125;
  }
  float4 outColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
  float4 outBlend = float4(0.0f, 0.0f, 0.0f, 0.0f);
  uint tint_symbol_111 = constant_shader();
  if ((tint_symbol_111 == 2u)) {
    float tint_symbol_112 = roundedBoxShadow((tint_symbol_3.data0.xy * 0.5f), tint_symbol_3.uv, asfloat(constants[2].y), tint_symbol_3.data1);
    outColor = (asfloat(constants[8]) * tint_symbol_112);
  } else {
    uint tint_symbol_114 = constant_shader();
    bool tint_symbol_113 = (tint_symbol_114 == 3u);
    if (!(tint_symbol_113)) {
      uint tint_symbol_115 = constant_shader();
      tint_symbol_113 = (tint_symbol_115 == 1u);
    }
    if (tint_symbol_113) {
      int sprite = tint_ftoi(tint_symbol_3.data0.z);
      uint stride = tint_ftou_1(tint_symbol_3.data0.w);
      int2 tuv = tint_ftoi_1(tint_symbol_3.uv);
      float4 shadeColor = computeShadeColor(tint_symbol_3.canvas_coord);
      if (useBlending()) {
        float3 tint_symbol_116 = atlasSubpixel(sprite, tuv, stride);
        float3 rgb = processSubpixelOutput(tint_symbol_116);
        outColor = (shadeColor * float4(rgb, 1.0f));
        outBlend = float4((shadeColor.a * rgb), 1.0f);
      } else {
        uint tint_symbol_117 = constant_shader();
        if ((tint_symbol_117 == 3u)) {
          float4 tint_symbol_118 = shadeColor;
          float4 tint_symbol_119 = atlasRGBA(sprite, tuv, stride);
          outColor = (tint_symbol_118 * tint_symbol_119);
        } else {
          float alpha = atlasAccum(sprite, tuv, stride);
          outColor = (shadeColor * float4((alpha).xxxx));
        }
      }
    } else {
      uint tint_symbol_120 = constant_shader();
      if ((tint_symbol_120 == 5u)) {
        uint2 xy = tint_ftou(tint_symbol_3.uv);
        float cov = tint_unpack4x8unorm(tint_symbol_3.coverage[(xy.y & 3u)])[(xy.x & 3u)];
        float4 shadeColor = computeShadeColor(tint_symbol_3.canvas_coord);
        outColor = (shadeColor * float4((cov).xxxx));
      } else {
        uint tint_symbol_121 = constant_shader();
        if ((tint_symbol_121 == 0u)) {
          float4 shadeColor = computeShadeColor(tint_symbol_3.canvas_coord);
          float4 rect = tint_symbol_3.data0;
          float pixelCoverage = rectangleCoverage(tint_symbol_3.canvas_coord, rect);
          outColor = (pixelCoverage * shadeColor);
        } else {
          outColor = float4(0.0f, 1.0f, 0.0f, 0.5f);
        }
      }
    }
  }
  FragOut tint_symbol_126 = {outColor, outBlend};
  return postprocessColor(tint_symbol_126, tint_symbol_3.canvas_coord);
}

tint_symbol_124 fragmentMain(tint_symbol_123 tint_symbol_122) {
  VertexOutput tint_symbol_127 = {float4(tint_symbol_122.position.xyz, (1.0f / tint_symbol_122.position.w)), tint_symbol_122.data0, tint_symbol_122.data1, tint_symbol_122.uv, tint_symbol_122.canvas_coord, tint_symbol_122.coverage};
  FragOut inner_result = fragmentMain_inner(tint_symbol_127);
  tint_symbol_124 wrapper_result = (tint_symbol_124)0;
  wrapper_result.color = inner_result.color;
  wrapper_result.blend = inner_result.blend;
  return wrapper_result;
}
