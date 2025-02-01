// 
// Brisk
// 
// Cross-platform application framework
// --------------------------------------------------------------
// 
// Copyright (C) 2024 Brisk Developers
// 
// This file is part of the Brisk library.
// 
// Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
// and a commercial license. You may use, modify, and distribute this software under
// the terms of the GPL-2.0+ license if you comply with its conditions.
// 
// You should have received a copy of the GNU General Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
// 
// If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
// license. For commercial licensing options, please visit: https://brisklib.com
// 
enable chromium_internal_dual_source_blending;

alias shader_type = i32;
alias gradient_type = i32;
alias subpixel_mode = i32;

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) @interpolate(linear) data0: vec4f,
    @location(1) @interpolate(linear) data1: vec4f,
    @location(2) @interpolate(linear, center) uv: vec2f,
    @location(3) @interpolate(linear, center) canvas_coord: vec2f,
};

const PI = 3.1415926535897932384626433832795;

const atlasAlignment     = 8;

const shader_rectangles  = shader_type(0);
const shader_arcs        = shader_type(1);
const shader_text        = shader_type(2);
const shader_shadow      = shader_type(3);
const shader_mask        = shader_type(4);

const gradient_linear    = gradient_type(0);
const gradient_radial    = gradient_type(1);
const gradient_angle     = gradient_type(2);
const gradient_reflected = gradient_type(3);

const subpixel_off       = subpixel_mode(0);
const subpixel_rgb       = subpixel_mode(1);
// const subpixel_bgr       = subpixel_mode(2);

/// Must match the value in Renderer.hpp
const gradientResolution = 1024;

struct UniformBlock {
    data_offset: u32,
    data_size: u32,
    instances: u32,
    unused: i32,

    shader: shader_type,
    texture_index: i32,
    scissors_border_radius: f32,
    scissors_corners: i32,

    coord_matrix_a: f32,
    coord_matrix_b: f32,
    coord_matrix_c: f32,
    coord_matrix_d: f32,

    coord_matrix_e: f32,
    coord_matrix_f: f32,
    sprite_oversampling: i32,
    subpixel: subpixel_mode,

    hpattern: u32,
    vpattern: u32,
    pattern_scale: i32,
    opacity: f32,

    scissor: vec4f,

    multigradient: i32,
    blur_directions: i32,
    texture_channel: i32,
    clipInScreenspace: i32,

    texture_matrix_a: f32,
    texture_matrix_b: f32,
    texture_matrix_c: f32,
    texture_matrix_d: f32,

    texture_matrix_e: f32,
    texture_matrix_f: f32,
    samplerMode: i32,
    blur_radius: f32,

    fill_color1: vec4f,

    fill_color2: vec4f,

    stroke_color1: vec4f,

    stroke_color2: vec4f,

    gradient_point1: vec2f,

    gradient_point2: vec2f,

    stroke_width: f32,
    gradient: gradient_type,
    shadow_flags: i32,
    reserved_5: f32,
}

struct UniformBlockPerFrame {
    viewport: vec4f,

    blue_light_filter: f32,
    global_gamma: f32,
    text_rect_padding: f32,

    text_rect_offset: f32,
    atlas_width: u32,
}

@group(0) @binding(1) var<uniform> constants: UniformBlock;
@group(0) @binding(2) var<uniform> perFrame: UniformBlockPerFrame;

@group(0) @binding(3) var<storage, read> data: array<vec4f>;

@group(0) @binding(8) var gradTex_t: texture_2d<f32>;

@group(0) @binding(9) var fontTex_t: texture_2d<f32>;

@group(0) @binding(10) var boundTexture_t: texture_2d<f32>;

@group(0) @binding(6) var boundTexture_s: sampler;

@group(0) @binding(7) var gradTex_s: sampler;

fn to_screen(xy: vec2f) -> vec2f {
    return xy * perFrame.viewport.zw * vec2f(2, -2) + vec2f(-1, 1);
}

fn from_screen(xy: vec2f) -> vec2f {
    return (xy - vec2f(-1, 1)) * vec2f(0.5, -0.5) * perFrame.viewport.xy;
}

fn max2(pt: vec2f) -> f32 {
    return max(pt.x, pt.y);
}

fn map(p1: vec2f, p2: vec2f) -> vec2f {
    return vec2f(p1.x * p2.x + p1.y * p2.y, p1.x * p2.y - p1.y * p2.x);
}

fn transform2D(pos: vec2f) -> vec2f {
    let coord_matrix = mat3x2f(constants.coord_matrix_a, constants.coord_matrix_b,
        constants.coord_matrix_c, constants.coord_matrix_d,
        constants.coord_matrix_e, constants.coord_matrix_f);
    return (coord_matrix * vec3f(pos, 1.0)).xy;
}

fn margin() -> f32 {
    return ceil(1.0 + constants.stroke_width * 0.5);
}

fn norm_rect(rect: vec4f) -> vec4f {
    return vec4f(min(rect.xy, rect.zw), max(rect.xy, rect.zw));
}

@vertex /**/fn vertexMain(@builtin(vertex_index) vidx: u32, @builtin(instance_index) inst: u32) -> VertexOutput {
    const vertices = array(vec2f(-0.5, -0.5), vec2f(0.5, -0.5), vec2f(-0.5, 0.5), vec2f(0.5, 0.5));
    var output: VertexOutput;

    let position = vertices[vidx];
    let uv_coord: vec2f = position + 0.5;
    var outPosition = vec4f(0);
    if constants.shader == shader_rectangles || constants.shader == shader_shadow {
        let m = margin();
        let rect = norm_rect(data[constants.data_offset + inst * 2]);
        output.data0 = vec4f(rect.zw - rect.xy, 0, 0);
        let dat = data[constants.data_offset + inst * 2 + 1];
        output.data1 = dat;

        let angle = dat.x;
        let angle_sin = sin(angle);
        let angle_cos = cos(angle);
        let center = (rect.xy + rect.zw) * 0.5;
        let pt = mix(rect.xy - vec2f(m), rect.zw + vec2f(m), uv_coord) - center;
        let pt2 = vec2f(angle_cos * pt.x - angle_sin * pt.y, angle_sin * pt.x + angle_cos * pt.y);
        outPosition = vec4f(pt2 + center, 0, 1);
        output.uv = position * (m + m + rect.zw - rect.xy);
    } else if constants.shader == shader_arcs {
        let m = margin();
        let dat0 = data[constants.data_offset + inst * 2];
        output.data0 = dat0;
        let dat1 = data[constants.data_offset + inst * 2 + 1];
        output.data1 = dat1;

        outPosition = vec4f(mix(dat0.xy - vec2f(dat0.z + m), dat0.xy + vec2f(dat0.z + m), uv_coord), 0, 1);
        output.uv = position * (m + m + 2.0 * dat0.z);
    } else if constants.shader == shader_text {
        var rect = norm_rect(data[constants.data_offset + inst * 2]);
        let glyph_data = data[constants.data_offset + inst * 2 + 1];

        let base = rect.x;

        rect.x += perFrame.text_rect_offset;
        rect.z += perFrame.text_rect_offset;

        rect.x -= perFrame.text_rect_padding;
        rect.z += perFrame.text_rect_padding;

        outPosition = vec4f(mix(rect.xy, rect.zw, uv_coord), 0, 1);
        output.uv = (outPosition.xy - vec2f(base, rect.y) + vec2f(-perFrame.text_rect_padding, 0)) * vec2f(f32(constants.sprite_oversampling), 1);
        output.data0 = glyph_data;
    } else if constants.shader == shader_mask {
        let rect = norm_rect(data[constants.data_offset + inst * 2]);
        let glyph_data = data[constants.data_offset + inst * 2 + 1];
        outPosition = vec4f(mix(rect.xy, rect.zw, uv_coord), 0, 1);
        output.uv = outPosition.xy - rect.xy;
        output.data0 = glyph_data;
    }

    output.canvas_coord = outPosition.xy;
    output.position = vec4f(to_screen(transform2D(outPosition.xy)), outPosition.zw);
    return output;
}

fn superlength(pt: vec2f) -> f32 {
    let p = 4.0;
    return pow(pow(pt.x, p) + pow(pt.y, p), 1.0 / p);
}

fn sd_length_ex(pt: vec2f, border_radius: f32) -> f32 {
    if border_radius == 0.0 {
        return max2(abs(pt));
    } else if border_radius > 0.0 {
        return length(pt);
    } else {
        return superlength(pt);
    }
}

fn sd_rectangle(pt: vec2f, rect_size: vec2f, border_radius: f32, corners: i32) -> f32 {
    let ext = rect_size * 0.5;
    let quadrant = u32(pt.x >= 0) + 2 * u32(pt.y >= 0);
    var rad: f32 = abs(border_radius);
    if (corners & (1 << quadrant)) == 0 {
        rad = 0.0;
    }
    let ext2 = ext - vec2(rad, rad);
    let d = abs(pt) - ext2;
    return (min(max(d.x, d.y), 0.0) + sd_length_ex(max(d, vec2f(0)), border_radius) - rad);
}

struct SignedDistance {
    sd: f32,
    intersect_sd: f32,
}


const EDGE_LEFT = 1;
const EDGE_TOP = 2;
const EDGE_RIGHT = 4;
const EDGE_BOTTOM = 8;

const EDGE_TOPLEFT = (EDGE_TOP | EDGE_LEFT);
const EDGE_TOPRIGHT = (EDGE_TOP | EDGE_RIGHT);
const EDGE_BOTTOMLEFT = (EDGE_BOTTOM | EDGE_LEFT);
const EDGE_BOTTOMRIGHT = (EDGE_BOTTOM | EDGE_RIGHT);

fn signedDistanceArc(pt: vec2f, outer_radius: f32, inner_radius: f32, start_angle: f32, end_angle: f32) -> SignedDistance {
    let outer_d = length(pt) - outer_radius;
    let inner_d = length(pt) - inner_radius;
    var circle = max(outer_d, -inner_d);

    if end_angle - start_angle < 2.0 * 3.141592653589793238 {
        let start_sincos = -vec2f(cos(start_angle), sin(start_angle));
        let end_sincos = vec2f(cos(end_angle), sin(end_angle));
        var pie: f32;
        let add = vec2f(dot(pt, start_sincos), dot(pt, end_sincos));
        if end_angle - start_angle > 3.141592653589793238 {
            pie = min(add.x, add.y); // union
        } else {
            pie = max(add.x, add.y); // intersect
        }
        circle = max(circle, pie);
    }
    return SignedDistance(circle, -1000);
}

fn signedDistanceRectangle(uv: vec2f, rectSize: vec2f, borderRadius: f32, corners: i32) -> SignedDistance {
    var sd: f32;
    var intersect_sd: f32 = -1000;

    sd = (sd_rectangle(uv, rectSize.xy, borderRadius, corners));

    if constants.stroke_width > 0.0 {
        let edges = corners >> 4; // same as corners
        if edges != 15 {
            intersect_sd = 1000; // off

            if (EDGE_LEFT & edges) != 0 { // left enabled
                intersect_sd = min(intersect_sd, uv.x + rectSize.x * 0.5 - constants.stroke_width * 0.5);
            }
            if (EDGE_TOP & edges) != 0 {  // top enabled
                intersect_sd = min(intersect_sd, uv.y + rectSize.y * 0.5 - constants.stroke_width * 0.5);
            }
            if (EDGE_RIGHT & edges) != 0 { // right enabled
                intersect_sd = min(intersect_sd, -uv.x + rectSize.x * 0.5 - constants.stroke_width * 0.5);
            }
            if (EDGE_BOTTOM & edges) != 0 { // bottom enabled
                intersect_sd = min(intersect_sd, -uv.y + rectSize.y * 0.5 - constants.stroke_width * 0.5);
            }

            if (EDGE_TOPLEFT & edges) == EDGE_TOPLEFT { // left & top enabled 
                intersect_sd = min(intersect_sd, max(uv.x, uv.y));
            }
            if (EDGE_TOPRIGHT & edges) == EDGE_TOPRIGHT { // right & top enabled
                intersect_sd = min(intersect_sd, max(-uv.x, uv.y));
            }
            if (EDGE_BOTTOMLEFT & edges) == EDGE_BOTTOMLEFT {  // left & bottom enabled
                intersect_sd = min(intersect_sd, max(uv.x, -uv.y));
            }
            if (EDGE_BOTTOMRIGHT & edges) == EDGE_BOTTOMRIGHT {  // right & bottom enabled
                intersect_sd = min(intersect_sd, max(-uv.x, -uv.y));
            }
        }
    }
    return SignedDistance(sd, intersect_sd);
}

struct Colors {
    brush: vec4f,
    stroke: vec4f,
}

fn gammaColor(color: vec4f) -> vec4f {
    return color;// pow(abs(color), vec4f(1.0 / 2.2));
}

fn simpleGradient(pos: f32, stroke: bool) -> vec4f {
    let color1 = select(constants.fill_color1, constants.stroke_color1, stroke);
    let color2 = select(constants.fill_color2, constants.stroke_color2, stroke);
    return mix(gammaColor(color1), gammaColor(color2), pos);
}

fn multiGradient(pos: f32) -> vec4f {
    let invDims = vec2f(1) / vec2f(textureDimensions(gradTex_t));
    return gammaColor(textureSample(gradTex_t, gradTex_s,
        vec2f(0.5 + pos * f32(gradientResolution - 1), 0.5 + f32(constants.multigradient)) * invDims));
}

fn remixColors(value: vec4f) -> vec4f {
    return constants.fill_color1 * value.x + constants.fill_color2 * value.y + constants.stroke_color1 * value.z + constants.stroke_color2 * value.w;
}

fn transformedTexCoord(uv: vec2f) -> vec2f {
    let texture_matrix = mat3x2f(constants.texture_matrix_a, constants.texture_matrix_b, constants.texture_matrix_c,
        constants.texture_matrix_d, constants.texture_matrix_e, constants.texture_matrix_f);

    let tex_size = vec2f(textureDimensions(boundTexture_t));
    var transformed_uv = (texture_matrix * vec3f(uv, 1.0)).xy;
    if constants.samplerMode == 0 {
        transformed_uv = clamp(transformed_uv, vec2f(0.5), tex_size - 0.5);
    }
    return transformed_uv / tex_size;
}

fn simpleCalcColors(canvas_coord: vec2f) -> Colors {
    var result: Colors;
    let grad_pos = gradientPosition(canvas_coord);
    if constants.multigradient == -1 {
        result.brush = simpleGradient(grad_pos, false);
    } else {
        result.brush = multiGradient(grad_pos);
    }
    result.stroke = simpleGradient(grad_pos, true);
    return result;
}

fn calcColors(canvas_coord: vec2f) -> Colors {
    var result: Colors;
    if constants.texture_index != -1 { // texture
        let transformed_uv = transformedTexCoord(canvas_coord);
        // if constants.blur_radius > 0.0 {
            // brush = sampleBlur(transformed_uv);
        // } else {
        result.brush = gammaColor(textureSample(boundTexture_t, boundTexture_s, transformed_uv));
        // }
        result.brush = clamp(result.brush, vec4f(0.0), vec4f(1.0));
        if constants.multigradient == -10 {
            result.brush = remixColors(result.brush);
        } else if constants.multigradient != -1 {
            result.brush = multiGradient(result.brush[constants.texture_channel]);
        }
        result.stroke = vec4(0.0);
    } else { // gradients
        result = simpleCalcColors(canvas_coord);
    }

    return result;
}

fn positionAlongLine(from_: vec2f, to: vec2f, point: vec2f) -> f32 {
    let dir = normalize(to - from_);
    let offs = point - from_;
    return dot(offs, dir) / length(to - from_);
}

fn getAngle(x: vec2f) -> f32 {
    return atan2(x.y, -x.x) / (2.0 * PI) + 0.5;
}

fn mapLine(from_: vec2f, to: vec2f, point: vec2f) -> vec3f {
    let len = length(to - from_);
    let dir = normalize(to - from_);
    return vec3f(map(point - from_, dir), len);
}

fn gradientPositionForPoint(point: vec2f) -> f32 {
    if constants.gradient == gradient_linear {
        return positionAlongLine(constants.gradient_point1, constants.gradient_point2, point);
    } else if constants.gradient == gradient_radial {
        return length(point - constants.gradient_point1) / length(constants.gradient_point2 - constants.gradient_point1);
    } else if constants.gradient == gradient_angle {
        return getAngle(mapLine(constants.gradient_point1, constants.gradient_point2, point).xy);
    } else if constants.gradient == gradient_reflected {
        return 1.0 - abs(fract(positionAlongLine(constants.gradient_point1, constants.gradient_point2, point)) * 2.0 - 1.0);
    } else {
        return 0.5;
    }
}

fn gradientPosition(canvas_coord: vec2f) -> f32 {
    let pos = gradientPositionForPoint(canvas_coord);
    return clamp(pos, 0.0, 1.0);
}

fn toCoverage(sd: f32) -> f32 {
    return clamp(0.5 - sd, 0.0, 1.0);
}

fn fillOnly(signed_distance: f32, colors: Colors) -> vec4f {
    let alpha = toCoverage(signed_distance);
    return colors.brush * alpha;
}

fn fillAndStroke(stroke_signed_distance: f32, mask_signed_distance: f32, colors: Colors) -> vec4f {
    let border_alpha = toCoverage(stroke_signed_distance);
    let mask_alpha = toCoverage(mask_signed_distance);
    return mix(colors.brush, colors.stroke, border_alpha) * mask_alpha;
}

fn signedDistanceToColor(sd: SignedDistance, canvas_coord: vec2f, uv: vec2f, rectSize: vec2f) -> vec4f {
    let colors: Colors = calcColors(canvas_coord);
    if constants.stroke_width > 0.0 {
        var stroke_sd = -(constants.stroke_width * 0.5 + sd.sd);
        stroke_sd = max(stroke_sd, sd.intersect_sd);
        return fillAndStroke(stroke_sd, sd.sd - constants.stroke_width * 0.5, colors);
    } else {
        return fillOnly(sd.sd, colors);
    }
}

fn get_pattern(x: u32, pattern: u32) -> u32 {
    return (pattern >> (x % u32(24))) & u32(1);
}

fn mask(pt: vec2f) -> f32 {
    let scissor_size = constants.scissor.zw - constants.scissor.xy;
    return toCoverage(sd_rectangle(pt - (constants.scissor.xy + scissor_size * 0.5), scissor_size,
        constants.scissors_border_radius, constants.scissors_corners));
}

fn atlas(sprite: i32, pos: vec2i, stride: u32) -> f32 {
    if pos.x < 0 || pos.x >= i32(stride) {
        return 0;
    }
    if sprite < 0 {
        return f32((pos.x & pos.y) & 1);
    }
    let linear = u32(sprite) * atlasAlignment + u32(pos.x) + u32(pos.y) * stride;
    return textureLoad(fontTex_t, vec2u(linear % perFrame.atlas_width, linear / perFrame.atlas_width), 0).r;
}

fn atlasAccum(sprite: i32, pos: vec2i, stride: u32) -> f32 {
    var alpha: f32 = 0;
    if constants.sprite_oversampling == 1 {
        alpha = atlas(sprite, pos, stride);
        return alpha;
    }
    for (var i = 0; i < constants.sprite_oversampling; i++) {
        alpha += atlas(sprite, pos + vec2i(i, 0), stride);
    }
    return alpha / f32(constants.sprite_oversampling);
}

fn atlasSubpixel(sprite: i32, pos: vec2i, stride: u32) -> vec3f {
    if constants.sprite_oversampling == 6 {
        let x0 = atlas(sprite, pos + vec2i(-2, 0), stride) + atlas(sprite, pos + vec2i(-1, 0), stride);
        let x1 = atlas(sprite, pos + vec2i(0, 0), stride) + atlas(sprite, pos + vec2i(1, 0), stride);
        let x2 = atlas(sprite, pos + vec2i(2, 0), stride) + atlas(sprite, pos + vec2i(3, 0), stride);
        let x3 = atlas(sprite, pos + vec2i(4, 0), stride) + atlas(sprite, pos + vec2i(5, 0), stride);
        let x4 = atlas(sprite, pos + vec2i(6, 0), stride) + atlas(sprite, pos + vec2i(7, 0), stride);
        let filt = vec3f(0.25, 0.5, 0.25) * 0.5;
        return vec3f(dot(vec3f(x0, x1, x2), filt), dot(vec3f(x1, x2, x3), filt), dot(vec3f(x2, x3, x4), filt));
    } else if constants.sprite_oversampling == 3 {
        let x0 = atlas(sprite, pos + vec2i(-2, 0), stride);
        let x1 = atlas(sprite, pos + vec2i(-1, 0), stride);
        let x2 = atlas(sprite, pos + vec2i(0, 0), stride);
        let x3 = atlas(sprite, pos + vec2i(1, 0), stride);
        let x4 = atlas(sprite, pos + vec2i(2, 0), stride);
        let x5 = atlas(sprite, pos + vec2i(3, 0), stride);
        let x6 = atlas(sprite, pos + vec2i(4, 0), stride);
        const filt = array<f32, 3>(0x08 / 256.0, 0x4D / 256.0, 0x56 / 256.0);
        return vec3f(
            x0 * filt[0] + x1 * filt[1] + x2 * filt[2] + x3 * filt[1] + x4 * filt[0],
            x1 * filt[0] + x2 * filt[1] + x3 * filt[2] + x4 * filt[1] + x5 * filt[0],
            x2 * filt[0] + x3 * filt[1] + x4 * filt[2] + x5 * filt[1] + x6 * filt[0]
        );
    } else {
        return vec3f(1);
    }
}

fn shadow(signed_distance: f32) -> vec4f {
    var op: f32 = 1;
    if (constants.shadow_flags & 1) == 0 && signed_distance < 0 {
        op = 0;
    }
    if (constants.shadow_flags & 2) == 0 && signed_distance > 0 {
        op = 0;
    }
    let shadow_size = constants.stroke_width / 2.0;
    var sh = (signed_distance + shadow_size * 0.25) / (shadow_size * 0.5);
    sh = clamp(exp(-sh * sh), 0.0, 1.0);
    let col = vec4f(constants.fill_color1 * sh) * sign(shadow_size);
    return op * col;
}

fn applyGamma(in: vec4f, gamma: f32) -> vec4f {
    return pow(max(in, vec4f(0)), vec4f(gamma));
}

fn applyBlueLightFilter(in: vec4f, intensity: f32) -> vec4f {
    return in * vec4f(1.0, 1.0 - intensity * 0.6 * 0.6, 1.0 - intensity * 0.6, 1.0);
}

struct FragOut {
    @location(0) @blend_src(0) color: vec4<f32>,
    @location(0) @blend_src(1) blend: vec4<f32>,
}

fn useBlending() -> bool {
    return (constants.shader == shader_text || constants.shader == shader_mask) && constants.subpixel != subpixel_off;
}

fn postprocessColor(in: FragOut, mask_value: f32, canvas_coord: vec2u) -> FragOut {
    var out: FragOut = in;
    var opacity = constants.opacity * mask_value;

    if (constants.hpattern | constants.vpattern) != 0 {
        let p = get_pattern(canvas_coord.x / u32(constants.pattern_scale), constants.hpattern) & get_pattern(canvas_coord.y / u32(constants.pattern_scale), constants.vpattern);
        opacity = opacity * f32(p);
    }
    out.color = out.color * opacity;
    if useBlending() {
        out.blend = out.blend * opacity;
    }

    if perFrame.blue_light_filter != 0 {
        out.color = applyBlueLightFilter(out.color, perFrame.blue_light_filter);
        if useBlending() {
            out.blend = applyBlueLightFilter(out.blend, perFrame.blue_light_filter);
        }
    }
    if perFrame.global_gamma != 1 {
        out.color = applyGamma(out.color, perFrame.global_gamma);
        if useBlending() {
            out.blend = applyGamma(out.blend, perFrame.global_gamma);
        }
    }
    if !useBlending() {
        out.blend = vec4f(out.color.a);
    }

    return out;
}

@fragment /**/fn fragmentMain(in: VertexOutput) -> FragOut {

    var pt: vec2<f32>;
    if constants.clipInScreenspace != 0 {
        pt = in.position.xy;
    } else {
        pt = in.canvas_coord;
    }
    let mask_value = mask(pt);
    if mask_value <= 0 {
        discard;
    }
    var outColor: vec4f;
    var outBlend: vec4f;
    var useBlend: bool = false;

    if constants.shader == shader_rectangles || constants.shader == shader_arcs {
        var sd: SignedDistance;
        if constants.shader == shader_rectangles {
            sd = signedDistanceRectangle(in.uv, in.data0.xy, in.data1.y, i32(in.data1.z));
        } else { // shader_arcs
            sd = signedDistanceArc(in.uv, in.data0.z, in.data0.w, in.data1.x, in.data1.y);
        }
        outColor = signedDistanceToColor(sd, in.canvas_coord, in.uv, in.data0.xy);
    } else if constants.shader == shader_shadow {
        outColor = shadow(sd_rectangle(in.uv, in.data0.xy, in.data1.y, i32(in.data1.z)));
    } else if constants.shader == shader_mask || constants.shader == shader_text {
        let sprite = i32(in.data0.z);
        let stride = u32(in.data0.w);
        let tuv = vec2i(in.uv);
        let colors: Colors = calcColors(in.canvas_coord);

        if useBlending() {
            let rgb = atlasSubpixel(sprite, tuv, stride);
            outColor = colors.brush * vec4f(rgb, 1);
            outBlend = vec4f(colors.brush.a * rgb, 1);
        } else {
            var alpha = atlasAccum(sprite, tuv, stride);
            outColor = colors.brush * vec4f(alpha);
        }
    }

    return postprocessColor(FragOut(outColor, outBlend), mask_value, vec2u(in.canvas_coord));
}
