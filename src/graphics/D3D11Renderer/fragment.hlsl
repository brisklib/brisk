static bool tint_discarded = false;

int tint_ftoi(float v) {
  return ((v < 2147483520.0f) ? ((v < -2147483648.0f) ? -2147483648 : int(v)) : 2147483647);
}

int2 tint_ftoi_1(float2 v) {
  return ((v < (2147483520.0f).xx) ? ((v < (-2147483648.0f).xx) ? (-2147483648).xx : int2(v)) : (2147483647).xx);
}

uint2 tint_ftou(float2 v) {
  return ((v < (4294967040.0f).xx) ? ((v < (0.0f).xx) ? (0u).xx : uint2(v)) : (4294967295u).xx);
}

uint tint_ftou_1(float v) {
  return ((v < 4294967040.0f) ? ((v < 0.0f) ? 0u : uint(v)) : 4294967295u);
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
  float2 tint_symbol_152 = map((tint_symbol - from_), dir);
  return float3(tint_symbol_152, len);
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
        float3 tint_symbol_153 = mapLine(asfloat(constants[13].xy), asfloat(constants[13].zw), tint_symbol);
        return getAngle(tint_symbol_153.xy);
      } else {
        if ((asint(constants[14].y) == 3)) {
          float tint_symbol_154 = positionAlongLine(asfloat(constants[13].xy), asfloat(constants[13].zw), tint_symbol);
          float tint_symbol_155 = frac(tint_symbol_154);
          float tint_symbol_156 = abs(((tint_symbol_155 * 2.0f) - 1.0f));
          return (1.0f - tint_symbol_156);
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

float4 sampleBlur(float2 pos) {
  uint2 tint_tmp_2;
  boundTexture_t.GetDimensions(tint_tmp_2.x, tint_tmp_2.y);
  uint2 texSize = tint_tmp_2;
  if ((constants[6].y == 3u)) {
    float2 scale = ((1.0f).xx / float2(texSize));
    switch(tint_ftoi(round((asfloat(constants[8].w) * 2.0f)))) {
      case 0:
      case 1: {
        return boundTexture_t.Sample(boundTexture_s, pos);
        break;
      }
      case 2: {
        float4 tint_symbol_7 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-0.01098699960857629776f).xx)));
        float4 tint_symbol_8 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.01098699960857629776f, 1.0f))));
        float4 tint_symbol_9 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.0f, -0.01098699960857629776f))));
        float4 tint_symbol_10 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (1.0f).xx)));
        return ((((tint_symbol_7 * 0.97838002443313598633f) + (tint_symbol_8 * 0.01074900012463331223f)) + (tint_symbol_9 * 0.01074900012463331223f)) + (tint_symbol_10 * 0.00011809999705292284f));
        break;
      }
      case 3: {
        float4 tint_symbol_11 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-1.00250005722045898438f).xx)));
        float4 tint_symbol_12 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.00250005722045898438f, 0.11919999867677688599f))));
        float4 tint_symbol_13 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.11919999867677688599f, -1.00250005722045898438f))));
        float4 tint_symbol_14 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (0.11919999867677688599f).xx)));
        float4 tint_symbol_15 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.11919999867677688599f, 2.0f))));
        float4 tint_symbol_16 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.0f, 0.11919999867677688599f))));
        return ((((((tint_symbol_11 * 0.01138800010085105896f) + (tint_symbol_12 * 0.09529799968004226685f)) + (tint_symbol_13 * 0.09529799968004226685f)) + (tint_symbol_14 * 0.79749000072479248047f)) + (tint_symbol_15 * 0.00023565000446978956f)) + (tint_symbol_16 * 0.00023565000446978956f));
        break;
      }
      case 4: {
        float4 tint_symbol_17 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-1.03310000896453857422f).xx)));
        float4 tint_symbol_18 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.03310000896453857422f, 0.24508999288082122803f))));
        float4 tint_symbol_19 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.03310000896453857422f, 2.00359988212585449219f))));
        float4 tint_symbol_20 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.24508999288082122803f, -1.03310000896453857422f))));
        float4 tint_symbol_21 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (0.24508999288082122803f).xx)));
        float4 tint_symbol_22 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.24508999288082122803f, 2.00359988212585449219f))));
        float4 tint_symbol_23 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.00359988212585449219f, -1.03310000896453857422f))));
        float4 tint_symbol_24 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.00359988212585449219f, 0.24508999288082122803f))));
        return ((((((((tint_symbol_17 * 0.04034899920225143433f) + (tint_symbol_18 * 0.15918999910354614258f)) + (tint_symbol_19 * 0.00133979995734989643f)) + (tint_symbol_20 * 0.15918999910354614258f)) + (tint_symbol_21 * 0.62803000211715698242f)) + (tint_symbol_22 * 0.00528590008616447449f)) + (tint_symbol_23 * 0.00133979995734989643f)) + (tint_symbol_24 * 0.00528590008616447449f));
        break;
      }
      case 5: {
        float4 tint_symbol_25 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-2.026599884033203125f).xx)));
        float4 tint_symbol_26 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.026599884033203125f, -0.32738998532295227051f))));
        float4 tint_symbol_27 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.026599884033203125f, 1.10339999198913574219f))));
        float4 tint_symbol_28 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.32738998532295227051f, -2.026599884033203125f))));
        float4 tint_symbol_29 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-0.32738998532295227051f).xx)));
        float4 tint_symbol_30 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.32738998532295227051f, 1.10339999198913574219f))));
        float4 tint_symbol_31 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.32738998532295227051f, 3.00640010833740234375f))));
        float4 tint_symbol_32 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.10339999198913574219f, -2.026599884033203125f))));
        float4 tint_symbol_33 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.10339999198913574219f, -0.32738998532295227051f))));
        float4 tint_symbol_34 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (1.10339999198913574219f).xx)));
        float4 tint_symbol_35 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.10339999198913574219f, 3.00640010833740234375f))));
        float4 tint_symbol_36 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.00640010833740234375f, -0.32738998532295227051f))));
        float4 tint_symbol_37 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.00640010833740234375f, 1.10339999198913574219f))));
        return (((((((((((((tint_symbol_25 * 0.00076219998300075531f) + (tint_symbol_26 * 0.0196499992161989212f)) + (tint_symbol_27 * 0.00717529980465769768f)) + (tint_symbol_28 * 0.0196499992161989212f)) + (tint_symbol_29 * 0.50660002231597900391f)) + (tint_symbol_30 * 0.18499000370502471924f)) + (tint_symbol_31 * 0.00052602001233026385f)) + (tint_symbol_32 * 0.00717529980465769768f)) + (tint_symbol_33 * 0.18499000370502471924f)) + (tint_symbol_34 * 0.0675470009446144104f)) + (tint_symbol_35 * 0.00019207999866921455f)) + (tint_symbol_36 * 0.00052602001233026385f)) + (tint_symbol_37 * 0.00019207999866921455f));
        break;
      }
      case 6: {
        float4 tint_symbol_38 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-2.07590007781982421875f).xx)));
        float4 tint_symbol_39 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.07590007781982421875f, -0.37753999233245849609f))));
        float4 tint_symbol_40 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.07590007781982421875f, 1.18239998817443847656f))));
        float4 tint_symbol_41 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.07590007781982421875f, 3.02929997444152832031f))));
        float4 tint_symbol_42 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.37753999233245849609f, -2.07590007781982421875f))));
        float4 tint_symbol_43 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-0.37753999233245849609f).xx)));
        float4 tint_symbol_44 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.37753999233245849609f, 1.18239998817443847656f))));
        float4 tint_symbol_45 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.37753999233245849609f, 3.02929997444152832031f))));
        float4 tint_symbol_46 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.18239998817443847656f, -2.07590007781982421875f))));
        float4 tint_symbol_47 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.18239998817443847656f, -0.37753999233245849609f))));
        float4 tint_symbol_48 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (1.18239998817443847656f).xx)));
        float4 tint_symbol_49 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.18239998817443847656f, 3.02929997444152832031f))));
        float4 tint_symbol_50 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.02929997444152832031f, -2.07590007781982421875f))));
        float4 tint_symbol_51 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.02929997444152832031f, -0.37753999233245849609f))));
        float4 tint_symbol_52 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.02929997444152832031f, 1.18239998817443847656f))));
        return (((((((((((((((tint_symbol_38 * 0.00341510004363954067f) + (tint_symbol_39 * 0.03746400028467178345f)) + (tint_symbol_40 * 0.01730000041425228119f)) + (tint_symbol_41 * 0.00026688000070862472f)) + (tint_symbol_42 * 0.03746400028467178345f)) + (tint_symbol_43 * 0.41098999977111816406f)) + (tint_symbol_44 * 0.18978999555110931396f)) + (tint_symbol_45 * 0.00292779994197189808f)) + (tint_symbol_46 * 0.01730000041425228119f)) + (tint_symbol_47 * 0.18978999555110931396f)) + (tint_symbol_48 * 0.08764100074768066406f)) + (tint_symbol_49 * 0.00135200005024671555f)) + (tint_symbol_50 * 0.00026688000070862472f)) + (tint_symbol_51 * 0.00292779994197189808f)) + (tint_symbol_52 * 0.00135200005024671555f));
        break;
      }
      case 7: {
        float4 tint_symbol_53 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-3.07100009918212890625f).xx)));
        float4 tint_symbol_54 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-3.07100009918212890625f, -1.24940001964569091797f))));
        float4 tint_symbol_55 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-3.07100009918212890625f, 0.40917998552322387695f))));
        float4 tint_symbol_56 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-3.07100009918212890625f, 2.13739991188049316406f))));
        float4 tint_symbol_57 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.24940001964569091797f, -3.07100009918212890625f))));
        float4 tint_symbol_58 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-1.24940001964569091797f).xx)));
        float4 tint_symbol_59 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.24940001964569091797f, 0.40917998552322387695f))));
        float4 tint_symbol_60 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.24940001964569091797f, 2.13739991188049316406f))));
        float4 tint_symbol_61 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.24940001964569091797f, 4.03539991378784179688f))));
        float4 tint_symbol_62 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.40917998552322387695f, -3.07100009918212890625f))));
        float4 tint_symbol_63 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.40917998552322387695f, -1.24940001964569091797f))));
        float4 tint_symbol_64 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (0.40917998552322387695f).xx)));
        float4 tint_symbol_65 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.40917998552322387695f, 2.13739991188049316406f))));
        float4 tint_symbol_66 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.40917998552322387695f, 4.03539991378784179688f))));
        float4 tint_symbol_67 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.13739991188049316406f, -3.07100009918212890625f))));
        float4 tint_symbol_68 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.13739991188049316406f, -1.24940001964569091797f))));
        float4 tint_symbol_69 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.13739991188049316406f, 0.40917998552322387695f))));
        float4 tint_symbol_70 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (2.13739991188049316406f).xx)));
        float4 tint_symbol_71 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(4.03539991378784179688f, -1.24940001964569091797f))));
        float4 tint_symbol_72 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(4.03539991378784179688f, 0.40917998552322387695f))));
        return ((((((((((((((((((((tint_symbol_53 * 0.0001820900070015341f) + (tint_symbol_54 * 0.00425769994035363197f)) + (tint_symbol_55 * 0.00781070021912455559f)) + (tint_symbol_56 * 0.00123079994227737188f)) + (tint_symbol_57 * 0.00425769994035363197f)) + (tint_symbol_58 * 0.0995519980788230896f)) + (tint_symbol_59 * 0.18263000249862670898f)) + (tint_symbol_60 * 0.02877900004386901855f)) + (tint_symbol_61 * 0.00031338000553660095f)) + (tint_symbol_62 * 0.00781070021912455559f)) + (tint_symbol_63 * 0.18263000249862670898f)) + (tint_symbol_64 * 0.33502998948097229004f)) + (tint_symbol_65 * 0.05279599875211715698f)) + (tint_symbol_66 * 0.00057489000027999282f)) + (tint_symbol_67 * 0.00123079994227737188f)) + (tint_symbol_68 * 0.02877900004386901855f)) + (tint_symbol_69 * 0.05279599875211715698f)) + (tint_symbol_70 * 0.00831979978829622269f)) + (tint_symbol_71 * 0.00031338000553660095f)) + (tint_symbol_72 * 0.00057489000027999282f));
        break;
      }
      case 8: {
        float4 tint_symbol_73 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-3.12249994277954101562f).xx)));
        float4 tint_symbol_74 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-3.12249994277954101562f, -1.30069994926452636719f))));
        float4 tint_symbol_75 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-3.12249994277954101562f, 0.43015000224113464355f))));
        float4 tint_symbol_76 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-3.12249994277954101562f, 2.19679999351501464844f))));
        float4 tint_symbol_77 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.30069994926452636719f, -3.12249994277954101562f))));
        float4 tint_symbol_78 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-1.30069994926452636719f).xx)));
        float4 tint_symbol_79 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.30069994926452636719f, 0.43015000224113464355f))));
        float4 tint_symbol_80 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.30069994926452636719f, 2.19679999351501464844f))));
        float4 tint_symbol_81 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-1.30069994926452636719f, 4.073699951171875f))));
        float4 tint_symbol_82 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.43015000224113464355f, -3.12249994277954101562f))));
        float4 tint_symbol_83 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.43015000224113464355f, -1.30069994926452636719f))));
        float4 tint_symbol_84 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (0.43015000224113464355f).xx)));
        float4 tint_symbol_85 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.43015000224113464355f, 2.19679999351501464844f))));
        float4 tint_symbol_86 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(0.43015000224113464355f, 4.073699951171875f))));
        float4 tint_symbol_87 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.19679999351501464844f, -3.12249994277954101562f))));
        float4 tint_symbol_88 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.19679999351501464844f, -1.30069994926452636719f))));
        float4 tint_symbol_89 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.19679999351501464844f, 0.43015000224113464355f))));
        float4 tint_symbol_90 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (2.19679999351501464844f).xx)));
        float4 tint_symbol_91 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(2.19679999351501464844f, 4.073699951171875f))));
        float4 tint_symbol_92 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(4.073699951171875f, -1.30069994926452636719f))));
        float4 tint_symbol_93 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(4.073699951171875f, 0.43015000224113464355f))));
        float4 tint_symbol_94 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(4.073699951171875f, 2.19679999351501464844f))));
        return ((((((((((((((((((((((tint_symbol_73 * 0.00073677999898791313f) + (tint_symbol_74 * 0.00877199973911046982f)) + (tint_symbol_75 * 0.01425999961793422699f)) + (tint_symbol_76 * 0.00328469998203217983f)) + (tint_symbol_77 * 0.00877199973911046982f)) + (tint_symbol_78 * 0.10444000363349914551f)) + (tint_symbol_79 * 0.16978000104427337646f)) + (tint_symbol_80 * 0.03910699859261512756f)) + (tint_symbol_81 * 0.00116029998753219843f)) + (tint_symbol_82 * 0.01425999961793422699f)) + (tint_symbol_83 * 0.16978000104427337646f)) + (tint_symbol_84 * 0.27599000930786132812f)) + (tint_symbol_85 * 0.06357300281524658203f)) + (tint_symbol_86 * 0.00188620004337280989f)) + (tint_symbol_87 * 0.00328469998203217983f)) + (tint_symbol_88 * 0.03910699859261512756f)) + (tint_symbol_89 * 0.06357300281524658203f)) + (tint_symbol_90 * 0.01464300043880939484f)) + (tint_symbol_91 * 0.00043446000199764967f)) + (tint_symbol_92 * 0.00116029998753219843f)) + (tint_symbol_93 * 0.00188620004337280989f)) + (tint_symbol_94 * 0.00043446000199764967f));
        break;
      }
      default: {
        float4 tint_symbol_95 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-4.11920022964477539062f, -2.24769997596740722656f))));
        float4 tint_symbol_96 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-4.11920022964477539062f, -0.44466999173164367676f))));
        float4 tint_symbol_97 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-4.11920022964477539062f, 1.33920001983642578125f))));
        float4 tint_symbol_98 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-4.11920022964477539062f, 3.17429995536804199219f))));
        float4 tint_symbol_99 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.24769997596740722656f, -4.11920022964477539062f))));
        float4 tint_symbol_100 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-2.24769997596740722656f).xx)));
        float4 tint_symbol_101 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.24769997596740722656f, -0.44466999173164367676f))));
        float4 tint_symbol_102 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.24769997596740722656f, 1.33920001983642578125f))));
        float4 tint_symbol_103 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.24769997596740722656f, 3.17429995536804199219f))));
        float4 tint_symbol_104 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-2.24769997596740722656f, 5.07980012893676757812f))));
        float4 tint_symbol_105 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.44466999173164367676f, -4.11920022964477539062f))));
        float4 tint_symbol_106 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.44466999173164367676f, -2.24769997596740722656f))));
        float4 tint_symbol_107 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (-0.44466999173164367676f).xx)));
        float4 tint_symbol_108 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.44466999173164367676f, 1.33920001983642578125f))));
        float4 tint_symbol_109 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.44466999173164367676f, 3.17429995536804199219f))));
        float4 tint_symbol_110 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(-0.44466999173164367676f, 5.07980012893676757812f))));
        float4 tint_symbol_111 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.33920001983642578125f, -4.11920022964477539062f))));
        float4 tint_symbol_112 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.33920001983642578125f, -2.24769997596740722656f))));
        float4 tint_symbol_113 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.33920001983642578125f, -0.44466999173164367676f))));
        float4 tint_symbol_114 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (1.33920001983642578125f).xx)));
        float4 tint_symbol_115 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.33920001983642578125f, 3.17429995536804199219f))));
        float4 tint_symbol_116 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(1.33920001983642578125f, 5.07980012893676757812f))));
        float4 tint_symbol_117 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.17429995536804199219f, -4.11920022964477539062f))));
        float4 tint_symbol_118 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.17429995536804199219f, -2.24769997596740722656f))));
        float4 tint_symbol_119 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.17429995536804199219f, -0.44466999173164367676f))));
        float4 tint_symbol_120 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(3.17429995536804199219f, 1.33920001983642578125f))));
        float4 tint_symbol_121 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * (3.17429995536804199219f).xx)));
        float4 tint_symbol_122 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(5.07980012893676757812f, -2.24769997596740722656f))));
        float4 tint_symbol_123 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(5.07980012893676757812f, -0.44466999173164367676f))));
        float4 tint_symbol_124 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * float2(5.07980012893676757812f, 1.33920001983642578125f))));
        return ((((((((((((((((((((((((((((((tint_symbol_95 * 0.00125410000327974558f) + (tint_symbol_96 * 0.00413249991834163666f)) + (tint_symbol_97 * 0.00278110010549426079f)) + (tint_symbol_98 * 0.00037614000029861927f)) + (tint_symbol_99 * 0.00125410000327974558f)) + (tint_symbol_100 * 0.0211299993097782135f)) + (tint_symbol_101 * 0.06963100284337997437f)) + (tint_symbol_102 * 0.04685999825596809387f)) + (tint_symbol_103 * 0.00633769994601607323f)) + (tint_symbol_104 * 0.00016245999722741544f)) + (tint_symbol_105 * 0.00413249991834163666f)) + (tint_symbol_106 * 0.06963100284337997437f)) + (tint_symbol_107 * 0.22946000099182128906f)) + (tint_symbol_108 * 0.15442000329494476318f)) + (tint_symbol_109 * 0.02088500000536441803f)) + (tint_symbol_110 * 0.00053536001360043883f)) + (tint_symbol_111 * 0.00278110010549426079f)) + (tint_symbol_112 * 0.04685999825596809387f)) + (tint_symbol_113 * 0.15442000329494476318f)) + (tint_symbol_114 * 0.10391999781131744385f)) + (tint_symbol_115 * 0.01405499968677759171f)) + (tint_symbol_116 * 0.00036028001341037452f)) + (tint_symbol_117 * 0.00037614000029861927f)) + (tint_symbol_118 * 0.00633769994601607323f)) + (tint_symbol_119 * 0.02088500000536441803f)) + (tint_symbol_120 * 0.01405499968677759171f)) + (tint_symbol_121 * 0.0019009000388905406f)) + (tint_symbol_122 * 0.00016245999722741544f)) + (tint_symbol_123 * 0.00053536001360043883f)) + (tint_symbol_124 * 0.00036028001341037452f));
        break;
      }
    }
  } else {
    float2 scale = (((constants[6].y == 1u) ? float2(1.0f, 0.0f) : float2(0.0f, 1.0f)) / float2(texSize));
    switch(tint_ftoi(round((asfloat(constants[8].w) * 2.0f)))) {
      case 0:
      case 1: {
        float4 tint_symbol_125 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -0.00000001523000037196f)));
        return (tint_symbol_125 * 1.0f);
        break;
      }
      case 2: {
        float4 tint_symbol_126 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -0.01098699960857629776f)));
        float4 tint_symbol_127 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 1.0f)));
        return ((tint_symbol_126 * 0.9891300201416015625f) + (tint_symbol_127 * 0.01086799986660480499f));
        break;
      }
      case 3: {
        float4 tint_symbol_128 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -1.00250005722045898438f)));
        float4 tint_symbol_129 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 0.11919999867677688599f)));
        float4 tint_symbol_130 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 2.0f)));
        return (((tint_symbol_128 * 0.10671000182628631592f) + (tint_symbol_129 * 0.8930199742317199707f)) + (tint_symbol_130 * 0.00026388000696897507f));
        break;
      }
      case 4: {
        float4 tint_symbol_131 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -1.03310000896453857422f)));
        float4 tint_symbol_132 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 0.24508999288082122803f)));
        float4 tint_symbol_133 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 2.00359988212585449219f)));
        return (((tint_symbol_131 * 0.20087000727653503418f) + (tint_symbol_132 * 0.79247999191284179688f)) + (tint_symbol_133 * 0.00667000003159046173f));
        break;
      }
      case 5: {
        float4 tint_symbol_134 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -2.026599884033203125f)));
        float4 tint_symbol_135 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -0.32738998532295227051f)));
        float4 tint_symbol_136 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 1.10339999198913574219f)));
        float4 tint_symbol_137 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 3.00640010833740234375f)));
        return ((((tint_symbol_134 * 0.02760799974203109741f) + (tint_symbol_135 * 0.71175998449325561523f)) + (tint_symbol_136 * 0.25990000367164611816f)) + (tint_symbol_137 * 0.00073904002783820033f));
        break;
      }
      case 6: {
        float4 tint_symbol_138 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -2.07590007781982421875f)));
        float4 tint_symbol_139 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -0.37753999233245849609f)));
        float4 tint_symbol_140 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 1.18239998817443847656f)));
        float4 tint_symbol_141 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 3.02929997444152832031f)));
        return ((((tint_symbol_138 * 0.05843900144100189209f) + (tint_symbol_139 * 0.64108997583389282227f)) + (tint_symbol_140 * 0.29603999853134155273f)) + (tint_symbol_141 * 0.00456689996644854546f));
        break;
      }
      case 7: {
        float4 tint_symbol_142 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -3.07100009918212890625f)));
        float4 tint_symbol_143 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -1.24940001964569091797f)));
        float4 tint_symbol_144 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 0.40917998552322387695f)));
        float4 tint_symbol_145 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 2.13739991188049316406f)));
        float4 tint_symbol_146 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 4.03539991378784179688f)));
        return (((((tint_symbol_142 * 0.01349399983882904053f) + (tint_symbol_143 * 0.3155199885368347168f)) + (tint_symbol_144 * 0.57881999015808105469f)) + (tint_symbol_145 * 0.09121300280094146729f)) + (tint_symbol_146 * 0.00099321000743657351f));
        break;
      }
      case 8: {
        float4 tint_symbol_147 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -3.12249994277954101562f)));
        float4 tint_symbol_148 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * -1.30069994926452636719f)));
        float4 tint_symbol_149 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 0.43015000224113464355f)));
        float4 tint_symbol_150 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 2.19679999351501464844f)));
        float4 tint_symbol_151 = boundTexture_t.Sample(boundTexture_s, (pos + (scale * 4.073699951171875f)));
        return (((((tint_symbol_147 * 0.02714399993419647217f) + (tint_symbol_148 * 0.32317000627517700195f)) + (tint_symbol_149 * 0.52534997463226318359f)) + (tint_symbol_150 * 0.12100999802350997925f)) + (tint_symbol_151 * 0.00359029998071491718f));
        break;
      }
      default: {
        float4 sum = (0.0f).xxxx;
        float s = ((asfloat(constants[8].w) * 0.3333333432674407959f) / 2.0f);
        int half_size = tint_ftoi((s * 2.5f));
        float g1 = (1.0f / sqrt(((6.28318548202514648438f * s) * s)));
        float g2 = (1.0f / ((2.0f * s) * s));
        int2 ipos = tint_ftoi_1((pos * float2(texSize)));
        sum = (boundTexture_t.Load(int3(ipos, 0)) * g1);
        int2 step = ((constants[6].y == 1u) ? int2(1, 0) : int2(0, 1));
        {
          for(int i = 1; (i <= half_size); i = (i + 1)) {
            float g = (g1 * exp((-(float((i * i))) * g2)));
            sum = (sum + ((boundTexture_t.Load(int3((ipos + (step * i)), 0)) + boundTexture_t.Load(int3((ipos - (step * i)), 0))) * g));
          }
        }
        return sum;
        break;
      }
    }
  }
}

Colors calcColors(float2 canvas_coord) {
  Colors result = (Colors)0;
  if ((asint(constants[1].y) != -1)) {
    float2 transformed_uv = transformedTexCoord(canvas_coord);
    if ((asfloat(constants[8].w) > 0.0f)) {
      result.brush = sampleBlur(transformed_uv);
    } else {
      result.brush = boundTexture_t.Sample(boundTexture_s, transformed_uv);
    }
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
  float tint_symbol_157 = sd;
  float tint_symbol_158 = distanceScale();
  return clamp((0.5f - (tint_symbol_157 * tint_symbol_158)), 0.0f, 1.0f);
}

float4 fillOnly(float signed_distance, Colors colors) {
  float4 tint_symbol_159 = colors.brush;
  float tint_symbol_160 = toCoverage(signed_distance);
  return (tint_symbol_159 * tint_symbol_160);
}

float4 blendNormalPremultiplied(float4 src, float4 tint_symbol_1) {
  return float4((src.rgb + (tint_symbol_1.rgb * (1.0f - src.a))), (src.a + (tint_symbol_1.a * (1.0f - src.a))));
}

float4 fillAndStroke(float stroke_sd, float fill_sd, Colors colors) {
  float4 tint_symbol_161 = colors.brush;
  float tint_symbol_162 = toCoverage(fill_sd);
  float4 backColor = (tint_symbol_161 * tint_symbol_162);
  float4 tint_symbol_163 = colors.stroke;
  float tint_symbol_164 = toCoverage(stroke_sd);
  float4 foreColor = (tint_symbol_163 * tint_symbol_164);
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
      float tint_symbol_165 = alpha;
      float tint_symbol_166 = atlas(sprite, (pos + int2(i, 0)), stride);
      alpha = (tint_symbol_165 + tint_symbol_166);
    }
  }
  return (alpha / float(asint(constants[4].z)));
}

float3 atlasSubpixel(int sprite, int2 pos, uint stride) {
  if ((asint(constants[4].z) == 6)) {
    float tint_symbol_167 = atlas(sprite, (pos + int2(-2, 0)), stride);
    float tint_symbol_168 = atlas(sprite, (pos + int2(-1, 0)), stride);
    float x0 = (tint_symbol_167 + tint_symbol_168);
    float tint_symbol_169 = atlas(sprite, (pos + (0).xx), stride);
    float tint_symbol_170 = atlas(sprite, (pos + int2(1, 0)), stride);
    float x1 = (tint_symbol_169 + tint_symbol_170);
    float tint_symbol_171 = atlas(sprite, (pos + int2(2, 0)), stride);
    float tint_symbol_172 = atlas(sprite, (pos + int2(3, 0)), stride);
    float x2 = (tint_symbol_171 + tint_symbol_172);
    float tint_symbol_173 = atlas(sprite, (pos + int2(4, 0)), stride);
    float tint_symbol_174 = atlas(sprite, (pos + int2(5, 0)), stride);
    float x3 = (tint_symbol_173 + tint_symbol_174);
    float tint_symbol_175 = atlas(sprite, (pos + int2(6, 0)), stride);
    float tint_symbol_176 = atlas(sprite, (pos + int2(7, 0)), stride);
    float x4 = (tint_symbol_175 + tint_symbol_176);
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
  float2 tint_symbol_177 = erf(((float2((x).xx) + float2(-(curved), curved)) * (0.70710676908493041992f / sigma)));
  float2 integral = (0.5f + (0.5f * tint_symbol_177));
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
      float tint_symbol_178 = value;
      float tint_symbol_179 = roundedBoxShadowX(tint_symbol.x, (tint_symbol.y - y), sigma, corner, halfSize);
      float tint_symbol_180 = gaussian(y, sigma);
      value = (tint_symbol_178 + ((tint_symbol_179 * tint_symbol_180) * step));
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
    uint tint_symbol_181 = samplePattern(tint_div(canvas_coord.x, pattern_scale), hpattern);
    uint tint_symbol_182 = samplePattern(tint_div(canvas_coord.y, pattern_scale), vpattern);
    uint p = (tint_symbol_181 & tint_symbol_182);
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

struct tint_symbol_187 {
  noperspective float4 data0 : TEXCOORD0;
  noperspective float4 data1 : TEXCOORD1;
  noperspective float2 uv : TEXCOORD2;
  noperspective float2 canvas_coord : TEXCOORD3;
  float4 position : SV_Position;
};
struct tint_symbol_188 {
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
      float tint_symbol_183 = roundedBoxShadow((tint_symbol_3.data0.xy * 0.5f), tint_symbol_3.uv, asfloat(constants[8].w), tint_symbol_3.data1);
      outColor = (asfloat(constants[9]) * tint_symbol_183);
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
            float4 tint_symbol_184 = colors.brush;
            float4 tint_symbol_185 = atlasRGBA(sprite, tuv, stride);
            outColor = (tint_symbol_184 * tint_symbol_185);
          } else {
            float alpha = atlasAccum(sprite, tuv, stride);
            outColor = (colors.brush * float4((alpha).xxxx));
          }
        }
      }
    }
  }
  FragOut tint_symbol_189 = {outColor, outBlend};
  return postprocessColor(tint_symbol_189, tint_ftou(tint_symbol_3.canvas_coord));
}

tint_symbol_188 fragmentMain(tint_symbol_187 tint_symbol_186) {
  VertexOutput tint_symbol_190 = {float4(tint_symbol_186.position.xyz, (1.0f / tint_symbol_186.position.w)), tint_symbol_186.data0, tint_symbol_186.data1, tint_symbol_186.uv, tint_symbol_186.canvas_coord};
  FragOut inner_result = fragmentMain_inner(tint_symbol_190);
  tint_symbol_188 wrapper_result = (tint_symbol_188)0;
  wrapper_result.color = inner_result.color;
  wrapper_result.blend = inner_result.blend;
  if (tint_discarded) {
    discard;
  }
  return wrapper_result;
}
