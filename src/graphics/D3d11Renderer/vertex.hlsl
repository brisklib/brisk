struct VertexOutput {
  float4 position;
  float4 data0;
  float4 data1;
  float2 uv;
  float2 canvas_coord;
  uint4 coverage;
};

cbuffer cbuffer_constants : register(b1) {
  uint4 constants[11];
};
cbuffer cbuffer_perFrame : register(b2) {
  uint4 perFrame[3];
};
ByteAddressBuffer data : register(t3);

uint constant_shader() {
  return uint((constants[1].x & 255u));
}

int constant_sprite_oversampling() {
  return int(((constants[1].y >> 24u) & 255u));
}

float4 get_data(uint index) {
  return asfloat(data.Load4((16u * (constants[0].x + index))));
}

float2 to_screen(float2 xy) {
  return (((xy * asfloat(perFrame[0]).zw) * float2(2.0f, -2.0f)) + float2(-1.0f, 1.0f));
}

float3x2 constants_load_1(uint offset) {
  const uint scalar_offset = ((offset + 0u)) / 4;
  uint4 ubo_load = constants[scalar_offset / 4];
  const uint scalar_offset_1 = ((offset + 8u)) / 4;
  uint4 ubo_load_1 = constants[scalar_offset_1 / 4];
  const uint scalar_offset_2 = ((offset + 16u)) / 4;
  uint4 ubo_load_2 = constants[scalar_offset_2 / 4];
  return float3x2(asfloat(((scalar_offset & 2) ? ubo_load.zw : ubo_load.xy)), asfloat(((scalar_offset_1 & 2) ? ubo_load_1.zw : ubo_load_1.xy)), asfloat(((scalar_offset_2 & 2) ? ubo_load_2.zw : ubo_load_2.xy)));
}

float2 transform2D(float2 pos) {
  return mul(float3(pos, 1.0f), constants_load_1(48u)).xy;
}

float margin() {
  return ceil((1.0f + ((asfloat(constants[2].y) / 0.18000000715255737305f) * 0.5f)));
}

float4 norm_rect(float4 rect) {
  return float4(min(rect.xy, rect.zw), max(rect.xy, rect.zw));
}

float4 alignRectangle(float4 rect) {
  return float4(floor(rect.xy), ceil(rect.zw));
}

struct tint_symbol_16 {
  uint vidx : SV_VertexID;
  uint inst : SV_InstanceID;
};
struct tint_symbol_17 {
  noperspective float4 data0 : TEXCOORD0;
  noperspective float4 data1 : TEXCOORD1;
  noperspective float2 uv : TEXCOORD2;
  noperspective float2 canvas_coord : TEXCOORD3;
  nointerpolation uint4 coverage : TEXCOORD4;
  float4 position : SV_Position;
};

VertexOutput vertexMain_inner(uint vidx, uint inst) {
  VertexOutput output = (VertexOutput)0;
  uint tint_symbol = constant_shader();
  if ((tint_symbol == 4u)) {
    float2 tint_symbol_18[4] = {(-0.5f).xx, float2(0.5f, -0.5f), float2(-0.5f, 0.5f), (0.5f).xx};
    output.position = float4((tint_symbol_18[vidx] * 2.0f), 0.0f, 1.0f);
    return output;
  }
  float2 tint_symbol_19[4] = {(-0.5f).xx, float2(0.5f, -0.5f), float2(-0.5f, 0.5f), (0.5f).xx};
  float2 position = tint_symbol_19[vidx];
  float2 uv_coord = (position + (0.5f).xx);
  float4 outPosition = (0.0f).xxxx;
  uint tint_symbol_1 = constant_shader();
  if ((tint_symbol_1 == 0u)) {
    float4 rect = get_data(inst);
    float4 alignedRect = alignRectangle(rect);
    float2 pt = lerp(alignedRect.xy, alignedRect.zw, uv_coord);
    output.data0 = rect;
    outPosition = float4(pt, 0.0f, 1.0f);
  } else {
    uint tint_symbol_2 = constant_shader();
    if ((tint_symbol_2 == 2u)) {
      float m = margin();
      float4 tint_symbol_3 = get_data((inst * 2u));
      float4 rect = norm_rect(tint_symbol_3);
      output.data0 = float4((rect.zw - rect.xy), 0.0f, 0.0f);
      float4 radii = get_data(((inst * 2u) + 1u));
      output.data1 = radii;
      float2 center = ((rect.xy + rect.zw) * 0.5f);
      float2 pt = lerp((rect.xy - float2((m).xx)), (rect.zw + float2((m).xx)), uv_coord);
      outPosition = float4(pt, 0.0f, 1.0f);
      output.uv = (position * (((m + m) + rect.zw) - rect.xy));
    } else {
      uint tint_symbol_4 = constant_shader();
      if ((tint_symbol_4 == 1u)) {
        float4 tint_symbol_5 = get_data((inst * 2u));
        float4 rect = norm_rect(tint_symbol_5);
        float4 glyph_data = get_data(((inst * 2u) + 1u));
        float base = rect.x;
        rect.x = (rect.x + asfloat(perFrame[1].w));
        rect.z = (rect.z + asfloat(perFrame[1].w));
        rect.x = (rect.x - asfloat(perFrame[1].z));
        rect.z = (rect.z + asfloat(perFrame[1].z));
        outPosition = float4(lerp(rect.xy, rect.zw, uv_coord), 0.0f, 1.0f);
        float2 tint_symbol_6 = ((outPosition.xy - float2(base, rect.y)) + float2(-(asfloat(perFrame[1].z)), 0.0f));
        int tint_symbol_7 = constant_sprite_oversampling();
        float tint_symbol_8 = float(tint_symbol_7);
        float2 tint_symbol_9 = float2(tint_symbol_8, 1.0f);
        output.uv = (tint_symbol_6 * tint_symbol_9);
        output.data0 = glyph_data;
      } else {
        uint tint_symbol_10 = constant_shader();
        if ((tint_symbol_10 == 3u)) {
          float4 tint_symbol_11 = get_data((inst * 2u));
          float4 rect = norm_rect(tint_symbol_11);
          float4 glyph_data = get_data(((inst * 2u) + 1u));
          outPosition = float4(lerp(rect.xy, rect.zw, uv_coord), 0.0f, 1.0f);
          output.uv = (outPosition.xy - rect.xy);
          output.data0 = glyph_data;
        } else {
          uint tint_symbol_12 = constant_shader();
          if ((tint_symbol_12 == 5u)) {
            uint4 d = data.Load4((16u * (constants[0].x + (inst >> 1u))));
            uint patchCoord = d[((inst & 1u) << 1u)];
            uint patchOffset = d[(((inst & 1u) << 1u) + 1u)];
            output.coverage = data.Load4((16u * ((constants[0].x + ((constants[0].z + 1u) >> 1u)) + patchOffset)));
            float2 xy = float2(uint2(((patchCoord & 4095u) * 4u), (((patchCoord >> 12u) & 4095u) * 4u)));
            outPosition = float4(lerp(xy, (xy + float2((4.0f * float((patchCoord >> 24u))), 4.0f)), uv_coord), 0.0f, 1.0f);
            output.uv = (outPosition.xy - xy);
          }
        }
      }
    }
  }
  output.canvas_coord = outPosition.xy;
  float2 tint_symbol_13 = transform2D(outPosition.xy);
  float2 tint_symbol_14 = to_screen(tint_symbol_13);
  output.position = float4(tint_symbol_14, outPosition.zw);
  return output;
}

tint_symbol_17 vertexMain(tint_symbol_16 tint_symbol_15) {
  VertexOutput inner_result = vertexMain_inner(tint_symbol_15.vidx, tint_symbol_15.inst);
  tint_symbol_17 wrapper_result = (tint_symbol_17)0;
  wrapper_result.position = inner_result.position;
  wrapper_result.data0 = inner_result.data0;
  wrapper_result.data1 = inner_result.data1;
  wrapper_result.uv = inner_result.uv;
  wrapper_result.canvas_coord = inner_result.canvas_coord;
  wrapper_result.coverage = inner_result.coverage;
  return wrapper_result;
}

