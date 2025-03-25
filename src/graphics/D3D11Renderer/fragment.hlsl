static bool tint_discarded = false;

int tint_ftoi(float v) {
  return ((v < 2147483520.0f) ? ((v < -2147483648.0f) ? -2147483648 : int(v)) : 2147483647);
}

uint2 tint_ftou(float2 v) {
  return ((v < (4294967040.0f).xx) ? ((v < (0.0f).xx) ? (0u).xx : uint2(v)) : (4294967295u).xx);
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

float distanceScale() {
  float2 ac = float2(asfloat(constants[3].x), asfloat(constants[3].z));
  return sqrt(dot(ac, ac));
}

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

bool isPointInQuad(float2 p, float2 q1, float2 q2, float2 q3, float2 q4) {
  float4 xx = float4(q1.x, q2.x, q3.x, q4.x);
  float4 yy = float4(q1.y, q2.y, q3.y, q4.y);
  bool4 result = ((((xx.yzwx - xx) * (p.y - yy)) - ((yy.yzwx - yy) * (p.x - xx))) > (0.0f).xxxx);
  return all(result);
}

bool scissorTest(float2 p) {
  float2 scissorPoint4 = (asfloat(constants[1].zw) + (asfloat(constants[2].zw) - asfloat(constants[2].xy)));
  return isPointInQuad(p, asfloat(constants[1].zw), asfloat(constants[2].xy), asfloat(constants[2].zw), scissorPoint4);
}

float superLength(float2 pt) {
  float p = 4.0f;
  return pow((pow(pt.x, p) + pow(pt.y, p)), (1.0f / p));
}

float sdLengthEx(float2 pt, float border_radius) {
  if ((border_radius == 0.0f)) {
    return max2(abs(pt));
  } else {
    if ((border_radius > 0.0f)) {
      return length(pt);
    } else {
      return superLength(pt);
    }
  }
}

float signedDistanceRectangle(float2 pt, float2 rect_size, float4 border_radii) {
  float2 ext = (rect_size * 0.5f);
  uint quadrant = (uint((pt.x >= 0.0f)) + (2u * uint((pt.y >= 0.0f))));
  float rad = abs(border_radii[quadrant]);
  float2 ext2 = (ext - float2(rad, rad));
  float2 d = (abs(pt) - ext2);
  float tint_symbol_5 = min(max(d.x, d.y), 0.0f);
  float tint_symbol_6 = sdLengthEx(max(d, (0.0f).xx), border_radii[quadrant]);
  return ((tint_symbol_5 + tint_symbol_6) - rad);
}

float signedDistanceArc(float2 pt, float outer_radius, float inner_radius, float start_angle, float end_angle) {
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
  return circle;
}

struct Colors {
  float4 brush;
  float4 stroke;
};

float4 simpleGradient(float pos, bool stroke) {
  float4 color1 = (stroke ? asfloat(constants[11]) : asfloat(constants[9]));
  float4 color2 = (stroke ? asfloat(constants[12]) : asfloat(constants[10]));
  return lerp(color1, color2, pos);
}

float4 multiGradient(float pos) {
  uint2 tint_tmp;
  gradTex_t.GetDimensions(tint_tmp.x, tint_tmp.y);
  float2 invDims = ((1.0f).xx / float2(tint_tmp));
  return gradTex_t.Sample(gradTex_s, (float2((0.5f + (pos * 1023.0f)), (0.5f + float(asint(constants[6].x)))) * invDims));
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

float sdfInfLine(float2 p, float2 p1, float2 p2) {
  float2 dir = normalize((p2 - p1));
  float2 normal = float2(-(dir.y), dir.x);
  return dot((p - p1), normal);
}

float angleGradient(float2 p, float2 p1, float2 p2) {
  float sd = sdfInfLine(p, p1, p2);
  float2 tint_symbol_21 = map((p - p1), normalize((p2 - p1)));
  float2 n = normalize(tint_symbol_21);
  float first = ((atan2(n.x, n.y) * 0.15915493667125701904f) + 0.75f);
  float second = ((atan2(-(n.x), -(n.y)) * 0.15915493667125701904f) + 0.25f);
  return lerp(first, second, clamp((sd + 0.5f), 0.0f, 1.0f));
}

float gradientPositionForPoint(float2 tint_symbol) {
  if ((asint(constants[14].y) == 0)) {
    return positionAlongLine(asfloat(constants[13].xy), asfloat(constants[13].zw), tint_symbol);
  } else {
    if ((asint(constants[14].y) == 1)) {
      return (length((tint_symbol - asfloat(constants[13].xy))) / length((asfloat(constants[13].zw) - asfloat(constants[13].xy))));
    } else {
      if ((asint(constants[14].y) == 2)) {
        return angleGradient(tint_symbol, asfloat(constants[13].xy), asfloat(constants[13].zw));
      } else {
        if ((asint(constants[14].y) == 3)) {
          float tint_symbol_22 = positionAlongLine(asfloat(constants[13].xy), asfloat(constants[13].zw), tint_symbol);
          float tint_symbol_23 = frac(tint_symbol_22);
          float tint_symbol_24 = abs(((tint_symbol_23 * 2.0f) - 1.0f));
          return (1.0f - tint_symbol_24);
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

float4 sqr4(float4 x) {
  return (x * x);
}

float2 conj(float2 xy) {
  return float2(xy.x, -(xy.y));
}

float4 sampleBlur(float2 pos) {
  uint2 tint_tmp_2;
  boundTexture_t.GetDimensions(tint_tmp_2.x, tint_tmp_2.y);
  int2 texSize = int2(tint_tmp_2);
  float sigma = asfloat(constants[8].w);
  int half_size = tint_ftoi(ceil((sigma * 3.0f)));
  int2 limit = (texSize - 1);
  int max_square = (half_size * half_size);
  float g1 = (1.0f / ((6.283184051513671875f * sigma) * sigma));
  float g2 = (-0.5f / (sigma * sigma));
  float2 w = (1.0f / float2(texSize));
  float2 lo = ((0.5f).xx / float2(texSize));
  float2 hi = (float2((float2(texSize) - 0.5f)) / float2(texSize));
  float tint_symbol_7 = g1;
  float4 tint_symbol_8 = boundTexture_t.Sample(boundTexture_s, pos);
  float4 sum = (tint_symbol_7 * tint_symbol_8);
  {
    for(int i = 1; (i <= half_size); i = (i + 2)) {
      {
        for(int j = 0; (j <= half_size); j = (j + 2)) {
          float tint_symbol_9 = g1;
          float4 tint_symbol_10 = sqr4(float2(float(i), float((i + 1))).xyxy);
          float4 tint_symbol_11 = sqr4(float2(float(j), float((j + 1))).xxyy);
          float4 tint_symbol_12 = exp(((tint_symbol_10 + tint_symbol_11) * g2));
          float4 abcd = (tint_symbol_9 * tint_symbol_12);
          float ab_sum = (abcd[0] + abcd[1]);
          float cd_sum = (abcd[2] + abcd[3]);
          float abcd_sum = (ab_sum + cd_sum);
          float ab_y = (abcd[1] / ab_sum);
          float cd_y = (abcd[3] / cd_sum);
          float abcd_y = (cd_sum / abcd_sum);
          float2 o = float2((((1.0f - abcd_y) * ab_y) + (abcd_y * cd_y)), abcd_y);
          float2 xy = (float2(float(i), float(j)) + o);
          float4 v1 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * xy)), lo, hi));
          float2 tint_symbol_13 = pos;
          float2 tint_symbol_14 = w;
          float2 tint_symbol_15 = conj(xy);
          float2 tint_symbol_16 = clamp((tint_symbol_13 + (tint_symbol_14 * tint_symbol_15.yx)), lo, hi);
          float4 v2 = boundTexture_t.Sample(boundTexture_s, tint_symbol_16);
          float4 v3 = boundTexture_t.Sample(boundTexture_s, clamp((pos + (w * -(xy))), lo, hi));
          float2 tint_symbol_17 = pos;
          float2 tint_symbol_18 = w;
          float2 tint_symbol_19 = conj(xy.yx);
          float2 tint_symbol_20 = clamp((tint_symbol_17 + (tint_symbol_18 * tint_symbol_19)), lo, hi);
          float4 v4 = boundTexture_t.Sample(boundTexture_s, tint_symbol_20);
          sum = (sum + ((((v1 + v2) + v3) + v4) * abcd_sum));
        }
      }
    }
  }
  return sum;
}

Colors calcColors(float2 canvas_coord) {
  if ((asint(constants[1].y) == -1)) {
    return simpleCalcColors(canvas_coord);
  }
  Colors result = (Colors)0;
  float2 transformed_uv = transformedTexCoord(canvas_coord);
  if ((asfloat(constants[8].w) > 0.0f)) {
    result.brush = sampleBlur(transformed_uv);
  } else {
    result.brush = boundTexture_t.Sample(boundTexture_s, transformed_uv);
  }
  result.brush = clamp(result.brush, (0.0f).xxxx, (1.0f).xxxx);
  if ((asint(constants[6].x) != -1)) {
    result.brush = multiGradient(result.brush[asint(constants[6].z)]);
  }
  result.stroke = (0.0f).xxxx;
  return result;
}

float toCoverage(float sd) {
  float tint_symbol_25 = sd;
  float tint_symbol_26 = distanceScale();
  return clamp((0.5f - (tint_symbol_25 * tint_symbol_26)), 0.0f, 1.0f);
}

float4 fillOnly(float signed_distance, Colors colors) {
  float4 tint_symbol_27 = colors.brush;
  float tint_symbol_28 = toCoverage(signed_distance);
  return (tint_symbol_27 * tint_symbol_28);
}

float4 blendNormalPremultiplied(float4 src, float4 tint_symbol_1) {
  return float4((src.rgb + (tint_symbol_1.rgb * (1.0f - src.a))), (src.a + (tint_symbol_1.a * (1.0f - src.a))));
}

float4 fillAndStroke(float stroke_sd, float fill_sd, Colors colors) {
  float4 tint_symbol_29 = colors.brush;
  float tint_symbol_30 = toCoverage(fill_sd);
  float4 backColor = (tint_symbol_29 * tint_symbol_30);
  float4 tint_symbol_31 = colors.stroke;
  float tint_symbol_32 = toCoverage(stroke_sd);
  float4 foreColor = (tint_symbol_31 * tint_symbol_32);
  return blendNormalPremultiplied(foreColor, backColor);
}

float4 signedDistanceToColor(float sd, float2 canvas_coord, float2 uv, float2 rectSize) {
  Colors colors = calcColors(canvas_coord);
  if ((asfloat(constants[14].x) > 0.0f)) {
    float stroke_sd = (abs(sd) - (asfloat(constants[14].x) * 0.5f));
    return fillAndStroke(stroke_sd, sd, colors);
  } else {
    return fillOnly(sd, colors);
  }
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
  bool tint_tmp_3 = (pos.x < 0);
  if (!tint_tmp_3) {
    tint_tmp_3 = (pos.x >= int(stride));
  }
  if ((tint_tmp_3)) {
    return 0.0f;
  }
  if ((sprite < 0)) {
    return float((uint((pos.x & pos.y)) & 1u));
  }
  uint tint_symbol_2 = (((uint(sprite) * 8u) + uint(pos.x)) + (uint(pos.y) * stride));
  return fontTex_t.Load(uint3(tint_mod(tint_symbol_2, perFrame[2].x), tint_div(tint_symbol_2, perFrame[2].x), 0u)).r;
}

float4 atlasRGBA(int sprite, int2 pos, uint stride) {
  bool tint_tmp_4 = (pos.x < 0);
  if (!tint_tmp_4) {
    tint_tmp_4 = (((pos.x * 4) + 3) >= int(stride));
  }
  if ((tint_tmp_4)) {
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
  if ((asint(constants[4].z) == 1)) {
    alpha = atlas(sprite, pos, stride);
    return alpha;
  }
  {
    for(int i = 0; (i < asint(constants[4].z)); i = (i + 1)) {
      float tint_symbol_33 = alpha;
      float tint_symbol_34 = atlas(sprite, (pos + int2(i, 0)), stride);
      alpha = (tint_symbol_33 + tint_symbol_34);
    }
  }
  return (alpha / float(asint(constants[4].z)));
}

float3 atlasSubpixel(int sprite, int2 pos, uint stride) {
  if ((asint(constants[4].z) == 6)) {
    float tint_symbol_35 = atlas(sprite, (pos + int2(-2, 0)), stride);
    float tint_symbol_36 = atlas(sprite, (pos + int2(-1, 0)), stride);
    float x0 = (tint_symbol_35 + tint_symbol_36);
    float tint_symbol_37 = atlas(sprite, (pos + (0).xx), stride);
    float tint_symbol_38 = atlas(sprite, (pos + int2(1, 0)), stride);
    float x1 = (tint_symbol_37 + tint_symbol_38);
    float tint_symbol_39 = atlas(sprite, (pos + int2(2, 0)), stride);
    float tint_symbol_40 = atlas(sprite, (pos + int2(3, 0)), stride);
    float x2 = (tint_symbol_39 + tint_symbol_40);
    float tint_symbol_41 = atlas(sprite, (pos + int2(4, 0)), stride);
    float tint_symbol_42 = atlas(sprite, (pos + int2(5, 0)), stride);
    float x3 = (tint_symbol_41 + tint_symbol_42);
    float tint_symbol_43 = atlas(sprite, (pos + int2(6, 0)), stride);
    float tint_symbol_44 = atlas(sprite, (pos + int2(7, 0)), stride);
    float x4 = (tint_symbol_43 + tint_symbol_44);
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
  float2 tint_symbol_45 = erf(((float2((x).xx) + float2(-(curved), curved)) * (0.70710676908493041992f / sigma)));
  float2 integral = (0.5f + (0.5f * tint_symbol_45));
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
      float tint_symbol_46 = value;
      float tint_symbol_47 = roundedBoxShadowX(tint_symbol.x, (tint_symbol.y - y), sigma, corner, halfSize);
      float tint_symbol_48 = gaussian(y, sigma);
      value = (tint_symbol_46 + ((tint_symbol_47 * tint_symbol_48) * step));
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
  bool tint_tmp_6 = (asint(constants[1].x) == 2);
  if (!tint_tmp_6) {
    tint_tmp_6 = (asint(constants[1].x) == 4);
  }
  bool tint_tmp_5 = (tint_tmp_6);
  if (tint_tmp_5) {
    tint_tmp_5 = (asint(constants[4].w) != 0);
  }
  return (tint_tmp_5);
}

FragOut postprocessColor(FragOut tint_symbol_3, uint2 canvas_coord) {
  FragOut tint_symbol_4 = tint_symbol_3;
  float opacity = asfloat(constants[5].w);
  if ((constants[5].x != 0u)) {
    uint pattern_scale = (constants[5].x >> 24u);
    uint hpattern = (constants[5].x & 4095u);
    uint vpattern = (constants[5].x >> 12u);
    uint tint_symbol_49 = samplePattern(tint_div(canvas_coord.x, pattern_scale), hpattern);
    uint tint_symbol_50 = samplePattern(tint_div(canvas_coord.y, pattern_scale), vpattern);
    uint p = (tint_symbol_49 & tint_symbol_50);
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
  if (!(useBlending())) {
    tint_symbol_4.blend = float4((tint_symbol_4.color.a).xxxx);
  }
  return tint_symbol_4;
}

struct tint_symbol_55 {
  noperspective float4 data0 : TEXCOORD0;
  noperspective float4 data1 : TEXCOORD1;
  noperspective float2 uv : TEXCOORD2;
  noperspective float2 canvas_coord : TEXCOORD3;
  float4 position : SV_Position;
};
struct tint_symbol_56 {
  float4 color : SV_Target0;
  float4 blend : SV_Target1;
};

FragOut fragmentMain_inner(VertexOutput tint_symbol_3) {
  if (!(scissorTest(tint_symbol_3.position.xy))) {
    tint_discarded = true;
  }
  float4 outColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
  float4 outBlend = float4(0.0f, 0.0f, 0.0f, 0.0f);
  bool useBlend = false;
  bool tint_tmp_7 = (asint(constants[1].x) == 0);
  if (!tint_tmp_7) {
    tint_tmp_7 = (asint(constants[1].x) == 1);
  }
  if ((tint_tmp_7)) {
    float sd = 0.0f;
    if ((asint(constants[1].x) == 0)) {
      sd = signedDistanceRectangle(tint_symbol_3.uv, tint_symbol_3.data0.xy, tint_symbol_3.data1);
    } else {
      sd = signedDistanceArc(tint_symbol_3.uv, tint_symbol_3.data0.z, tint_symbol_3.data0.w, tint_symbol_3.data1.x, tint_symbol_3.data1.y);
    }
    outColor = signedDistanceToColor(sd, tint_symbol_3.canvas_coord, tint_symbol_3.uv, tint_symbol_3.data0.xy);
  } else {
    if ((asint(constants[1].x) == 3)) {
      float tint_symbol_51 = roundedBoxShadow((tint_symbol_3.data0.xy * 0.5f), tint_symbol_3.uv, asfloat(constants[8].w), tint_symbol_3.data1);
      outColor = (asfloat(constants[9]) * tint_symbol_51);
    } else {
      bool tint_tmp_9 = (asint(constants[1].x) == 4);
      if (!tint_tmp_9) {
        tint_tmp_9 = (asint(constants[1].x) == 5);
      }
      bool tint_tmp_8 = (tint_tmp_9);
      if (!tint_tmp_8) {
        tint_tmp_8 = (asint(constants[1].x) == 2);
      }
      if ((tint_tmp_8)) {
        int sprite = tint_ftoi(tint_symbol_3.data0.z);
        uint stride = tint_ftou_1(tint_symbol_3.data0.w);
        int2 tuv = tint_ftoi_1(tint_symbol_3.uv);
        Colors colors = calcColors(tint_symbol_3.canvas_coord);
        if (useBlending()) {
          float3 rgb = atlasSubpixel(sprite, tuv, stride);
          outColor = (colors.brush * float4(rgb, 1.0f));
          outBlend = float4((colors.brush.a * rgb), 1.0f);
        } else {
          if ((asint(constants[1].x) == 5)) {
            float4 tint_symbol_52 = colors.brush;
            float4 tint_symbol_53 = atlasRGBA(sprite, tuv, stride);
            outColor = (tint_symbol_52 * tint_symbol_53);
          } else {
            float alpha = atlasAccum(sprite, tuv, stride);
            outColor = (colors.brush * float4((alpha).xxxx));
          }
        }
      }
    }
  }
  FragOut tint_symbol_57 = {outColor, outBlend};
  return postprocessColor(tint_symbol_57, tint_ftou(tint_symbol_3.canvas_coord));
}

tint_symbol_56 fragmentMain(tint_symbol_55 tint_symbol_54) {
  VertexOutput tint_symbol_58 = {float4(tint_symbol_54.position.xyz, (1.0f / tint_symbol_54.position.w)), tint_symbol_54.data0, tint_symbol_54.data1, tint_symbol_54.uv, tint_symbol_54.canvas_coord};
  FragOut inner_result = fragmentMain_inner(tint_symbol_58);
  tint_symbol_56 wrapper_result = (tint_symbol_56)0;
  wrapper_result.color = inner_result.color;
  wrapper_result.blend = inner_result.blend;
  if (tint_discarded) {
    discard;
  }
  return wrapper_result;
}
