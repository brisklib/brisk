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
ByteAddressBuffer data : register(t3);

float4 get_data(uint index) {
  return asfloat(data.Load4((16u * (constants[0].x + index))));
}

float2 to_screen(float2 xy) {
  return (((xy * asfloat(perFrame[0]).zw) * float2(2.0f, -2.0f)) + float2(-1.0f, 1.0f));
}

float2 transform2D(float2 pos) {
  float3x2 coord_matrix = float3x2(float2(asfloat(constants[3].x), asfloat(constants[3].y)), float2(asfloat(constants[3].z), asfloat(constants[3].w)), float2(asfloat(constants[4].x), asfloat(constants[4].y)));
  return mul(float3(pos, 1.0f), coord_matrix).xy;
}

float margin() {
  if ((asint(constants[1].x) == 2)) {
    return ceil((1.0f + ((asfloat(constants[8].w) / 0.18000000715255737305f) * 0.5f)));
  } else {
    return ceil((1.0f + (asfloat(constants[12].x) * 0.5f)));
  }
}

float4 norm_rect(float4 rect) {
  return float4(min(rect.xy, rect.zw), max(rect.xy, rect.zw));
}

struct tint_symbol_6 {
  uint vidx : SV_VertexID;
  uint inst : SV_InstanceID;
};
struct tint_symbol_7 {
  noperspective float4 data0 : TEXCOORD0;
  noperspective float4 data1 : TEXCOORD1;
  noperspective float2 uv : TEXCOORD2;
  noperspective float2 canvas_coord : TEXCOORD3;
  nointerpolation uint4 coverage : TEXCOORD4;
  float4 position : SV_Position;
};

VertexOutput vertexMain_inner(uint vidx, uint inst) {
  VertexOutput output = (VertexOutput)0;
  if ((asint(constants[1].x) == 4)) {
    float2 tint_symbol_8[4] = {(-0.5f).xx, float2(0.5f, -0.5f), float2(-0.5f, 0.5f), (0.5f).xx};
    output.position = float4((tint_symbol_8[vidx] * 2.0f), 0.0f, 1.0f);
    return output;
  }
  float2 tint_symbol_9[4] = {(-0.5f).xx, float2(0.5f, -0.5f), float2(-0.5f, 0.5f), (0.5f).xx};
  float2 position = tint_symbol_9[vidx];
  float2 uv_coord = (position + (0.5f).xx);
  float4 outPosition = (0.0f).xxxx;
  if ((asint(constants[1].x) == 2)) {
    float m = margin();
    float4 tint_symbol_2 = get_data((inst * 2u));
    float4 rect = norm_rect(tint_symbol_2);
    output.data0 = float4((rect.zw - rect.xy), 0.0f, 0.0f);
    float4 radii = get_data(((inst * 2u) + 1u));
    output.data1 = radii;
    float2 center = ((rect.xy + rect.zw) * 0.5f);
    float2 pt = lerp((rect.xy - float2((m).xx)), (rect.zw + float2((m).xx)), uv_coord);
    outPosition = float4(pt, 0.0f, 1.0f);
    output.uv = (position * (((m + m) + rect.zw) - rect.xy));
  } else {
    if ((asint(constants[1].x) == 1)) {
      float4 tint_symbol_3 = get_data((inst * 2u));
      float4 rect = norm_rect(tint_symbol_3);
      float4 glyph_data = get_data(((inst * 2u) + 1u));
      float base = rect.x;
      rect.x = (rect.x + asfloat(perFrame[1].w));
      rect.z = (rect.z + asfloat(perFrame[1].w));
      rect.x = (rect.x - asfloat(perFrame[1].z));
      rect.z = (rect.z + asfloat(perFrame[1].z));
      outPosition = float4(lerp(rect.xy, rect.zw, uv_coord), 0.0f, 1.0f);
      output.uv = (((outPosition.xy - float2(base, rect.y)) + float2(-(asfloat(perFrame[1].z)), 0.0f)) * float2(float(asint(constants[4].z)), 1.0f));
      output.data0 = glyph_data;
    } else {
      if ((asint(constants[1].x) == 3)) {
        float4 tint_symbol_4 = get_data((inst * 2u));
        float4 rect = norm_rect(tint_symbol_4);
        float4 glyph_data = get_data(((inst * 2u) + 1u));
        outPosition = float4(lerp(rect.xy, rect.zw, uv_coord), 0.0f, 1.0f);
        output.uv = (outPosition.xy - rect.xy);
        output.data0 = glyph_data;
      } else {
        if ((asint(constants[1].x) == 5)) {
          uint4 d = data.Load4((16u * (constants[0].x + (inst >> 1u))));
          uint patchXY = d[((inst & 1u) << 1u)];
          uint patchOffset = d[(((inst & 1u) << 1u) + 1u)];
          output.coverage = data.Load4((16u * ((constants[0].x + ((constants[0].z + 1u) >> 1u)) + patchOffset)));
          float2 xy = float2(uint2((patchXY & 65535u), (patchXY >> 16u)));
          outPosition = float4(lerp(xy, (xy + (4.0f).xx), uv_coord), 0.0f, 1.0f);
          output.uv = (outPosition.xy - xy);
        }
      }
    }
  }
  output.canvas_coord = outPosition.xy;
  float2 tint_symbol = transform2D(outPosition.xy);
  float2 tint_symbol_1 = to_screen(tint_symbol);
  output.position = float4(tint_symbol_1, outPosition.zw);
  return output;
}

tint_symbol_7 vertexMain(tint_symbol_6 tint_symbol_5) {
  VertexOutput inner_result = vertexMain_inner(tint_symbol_5.vidx, tint_symbol_5.inst);
  tint_symbol_7 wrapper_result = (tint_symbol_7)0;
  wrapper_result.position = inner_result.position;
  wrapper_result.data0 = inner_result.data0;
  wrapper_result.data1 = inner_result.data1;
  wrapper_result.uv = inner_result.uv;
  wrapper_result.canvas_coord = inner_result.canvas_coord;
  wrapper_result.coverage = inner_result.coverage;
  return wrapper_result;
}
