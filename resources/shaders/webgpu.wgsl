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
enable dual_source_blending;

alias shader_type = i32;
alias gradient_type = i32;
alias subpixel_mode = i32;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) @interpolate(linear) data0: vec4<f32>,
    @location(1) @interpolate(linear) data1: vec4<f32>,
    @location(2) @interpolate(linear, center) uv: vec2<f32>,
    @location(3) @interpolate(linear, center) canvas_coord: vec2<f32>,
    @location(4) @interpolate(flat) coverage: vec4<u32>,
};

const PI = 3.1415926535897932384626433832795;

const atlasAlignment     = 8u;

const shader_rectangle   = shader_type(0);
const shader_text        = shader_type(1);
const shader_shadow      = shader_type(2);
const shader_color_mask  = shader_type(3);
const shader_blit        = shader_type(4);
const shader_mask        = shader_type(5);

const gradient_linear    = gradient_type(0);
const gradient_radial    = gradient_type(1);
const gradient_angle     = gradient_type(2);
const gradient_reflected = gradient_type(3);

const subpixel_off       = subpixel_mode(0);
const subpixel_rgb       = subpixel_mode(1);
const subpixel_bgr       = subpixel_mode(2);

/// Must match the value in Renderer.hpp
const gradientResolution = 1024;

struct UniformBlock {
    data_offset: u32,
    data_size: u32,
    instances: u32,
    unused: i32,

    shader: shader_type,
    texture_index: i32,

    reserved_00: vec2<f32>,
    reserved_01: vec2<f32>,
    reserved_02: vec2<f32>,

    coord_matrix_a: f32,
    coord_matrix_b: f32,
    coord_matrix_c: f32,
    coord_matrix_d: f32,

    coord_matrix_e: f32,
    coord_matrix_f: f32,
    sprite_oversampling: i32,
    subpixel: subpixel_mode,

    pattern: u32,
    reserved_1: u32,
    reserved_2: i32,
    opacity: f32,

    multigradient: i32,
    blur_directions: u32,
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

    fill_color1: vec4<f32>,

    fill_color2: vec4<f32>,

    gradient_point1: vec2<f32>,

    gradient_point2: vec2<f32>,

    stroke_width: f32,
    gradient: gradient_type,
}

struct UniformBlockPerFrame {
    viewport: vec4<f32>,

    blue_light_filter: f32,
    global_gamma: f32,
    text_rect_padding: f32,

    text_rect_offset: f32,
    atlas_width: u32,
}

@group(0) @binding(1) var<uniform> constants: UniformBlock;
@group(0) @binding(2) var<uniform> perFrame: UniformBlockPerFrame;

@group(0) @binding(3) var<storage, read> data: array<vec4<u32>>;

@group(0) @binding(8) var gradTex_t: texture_2d<f32>;

@group(0) @binding(9) var fontTex_t: texture_2d<f32>;

@group(0) @binding(10) var boundTexture_t: texture_2d<f32>;

@group(0) @binding(6) var boundTexture_s: sampler;

@group(0) @binding(7) var gradTex_s: sampler;

fn get_data(index: u32) -> vec4<f32> {
    return bitcast<vec4<f32>>(data[constants.data_offset + index]);
}

fn to_screen(xy: vec2<f32>) -> vec2<f32> {
    return xy * perFrame.viewport.zw * vec2<f32>(2., -2.) + vec2<f32>(-1., 1.);
}

fn map(p1: vec2<f32>, p2: vec2<f32>) -> vec2<f32> {
    return vec2<f32>(p1.x * p2.x + p1.y * p2.y, p1.x * p2.y - p1.y * p2.x);
}

// Function to check if point p is inside convex quad defined by q1, q2, q3, q4
fn isPointInQuad(p: vec2<f32>, q1: vec2<f32>, q2: vec2<f32>, q3: vec2<f32>, q4: vec2<f32>) -> bool {
    let xx = vec4<f32>(q1.x, q2.x, q3.x, q4.x);
    let yy = vec4<f32>(q1.y, q2.y, q3.y, q4.y);
    let result = (xx.yzwx - xx) * (p.y - yy) - (yy.yzwx - yy) * (p.x - xx) > vec4<f32>(0.0);
    return all(result);
}

fn transform2D(pos: vec2<f32>) -> vec2<f32> {
    let coord_matrix = mat3x2<f32>(constants.coord_matrix_a, constants.coord_matrix_b,
        constants.coord_matrix_c, constants.coord_matrix_d,
        constants.coord_matrix_e, constants.coord_matrix_f);
    return (coord_matrix * vec3<f32>(pos, 1.0)).xy;
}

fn margin() -> f32 {
    if constants.shader == shader_shadow {
        return ceil(1.0 + constants.blur_radius / 0.18 * 0.5);
    } else {
        return ceil(1.0 + constants.stroke_width * 0.5);
    }
}

fn norm_rect(rect: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(min(rect.xy, rect.zw), max(rect.xy, rect.zw));
}

fn alignRectangle(rect: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(
        floor(rect.xy),
        ceil(rect.zw)
    );
}

@vertex /**/fn vertexMain(@builtin(vertex_index) vidx: u32, @builtin(instance_index) inst: u32) -> VertexOutput {
    const vertices = array<vec2<f32>, 4>(vec2<f32>(-0.5, -0.5), vec2<f32>(0.5, -0.5), vec2<f32>(-0.5, 0.5), vec2<f32>(0.5, 0.5));

    var output: VertexOutput;
    if constants.shader == shader_blit {
        output.position = vec4(vertices[vidx] * 2.0, 0.0, 1.0);
        return output;
    }

    let position: vec2<f32> = vertices[vidx];
    let uv_coord: vec2<f32> = position + vec2<f32>(0.5);
    var outPosition = vec4<f32>(0.);
    if constants.shader == shader_rectangle {
        let rect = get_data(inst);
        let alignedRect = alignRectangle(rect);
        let pt = mix(alignedRect.xy, alignedRect.zw, uv_coord);
        output.data0 = rect;
        outPosition = vec4<f32>(pt, 0., 1.);
    } else if constants.shader == shader_shadow {
        let m: f32 = margin();
        let rect = norm_rect(get_data(inst * 2u));
        output.data0 = vec4<f32>(rect.zw - rect.xy, 0., 0.);
        let radii = get_data(inst * 2u + 1u);
        output.data1 = radii;
        let center = (rect.xy + rect.zw) * 0.5;
        let pt = mix(rect.xy - vec2<f32>(m), rect.zw + vec2<f32>(m), uv_coord);
        outPosition = vec4<f32>(pt, 0., 1.);
        output.uv = position * (m + m + rect.zw - rect.xy);
    } else if constants.shader == shader_text {
        var rect = norm_rect(get_data(inst * 2u));
        let glyph_data = get_data(inst * 2u + 1u);

        let base = rect.x;

        rect.x += perFrame.text_rect_offset;
        rect.z += perFrame.text_rect_offset;

        rect.x -= perFrame.text_rect_padding;
        rect.z += perFrame.text_rect_padding;

        outPosition = vec4<f32>(mix(rect.xy, rect.zw, uv_coord), 0., 1.);
        output.uv = (outPosition.xy - vec2<f32>(base, rect.y) + vec2<f32>(-perFrame.text_rect_padding, 0.)) * vec2<f32>(f32(constants.sprite_oversampling), 1.);
        output.data0 = glyph_data;
    } else if constants.shader == shader_color_mask {
        let rect = norm_rect(get_data(inst * 2u));
        let glyph_data = get_data(inst * 2u + 1u);
        outPosition = vec4<f32>(mix(rect.xy, rect.zw, uv_coord), 0., 1.);
        output.uv = outPosition.xy - rect.xy;
        output.data0 = glyph_data;
    } else if constants.shader == shader_mask {
        let d = data[constants.data_offset + (inst >> 1u)];
        let patchCoord: u32 = d[(inst & 1u) << 1u];
        let patchOffset: u32 = d[((inst & 1u) << 1u) + 1u];
        output.coverage = data[constants.data_offset + ((constants.instances + 1u) >> 1u) + patchOffset];
        let xy = vec2<f32>(vec2<u32>((patchCoord & 0xfffu) * 4u, ((patchCoord >> 12u) & 0xfffu) * 4u));
        outPosition = vec4<f32>(mix(xy, xy + vec2<f32>(4. * f32(patchCoord >> 24u), 4.), uv_coord), 0., 1.);
        output.uv = outPosition.xy - xy;
    }

    output.canvas_coord = outPosition.xy;
    output.position = vec4<f32>(to_screen(transform2D(outPosition.xy)), outPosition.zw);
    return output;
}

fn simpleGradient(pos: f32) -> vec4<f32> {
    return mix(constants.fill_color1, constants.fill_color2, pos);
}

fn multiGradient(pos: f32) -> vec4<f32> {
    let invDims = vec2<f32>(1.) / vec2<f32>(textureDimensions(gradTex_t));
    return textureSample(gradTex_t, gradTex_s,
        vec2<f32>(0.5 + pos * f32(gradientResolution - 1), 0.5 + f32(constants.multigradient)) * invDims);
}

fn transformedTexCoord(uv: vec2<f32>) -> vec2<f32> {
    let texture_matrix = mat3x2<f32>(constants.texture_matrix_a, constants.texture_matrix_b, constants.texture_matrix_c,
        constants.texture_matrix_d, constants.texture_matrix_e, constants.texture_matrix_f);

    let tex_size = vec2<f32>(textureDimensions(boundTexture_t));
    var transformed_uv = (texture_matrix * vec3<f32>(uv, 1.0)).xy;
    if constants.samplerMode == 0 {
        transformed_uv = clamp(transformed_uv, vec2<f32>(0.5), tex_size - 0.5);
    }
    return transformed_uv / tex_size;
}

fn sqr(x: f32) -> f32 {
    return x * x;
}

fn sqr4(x: vec4<f32>) -> vec4<f32> {
    return x * x;
}

fn conj(xy: vec2<f32>) -> vec2<f32> {
    return vec2<f32>(xy.x, -xy.y);
}

fn sampleBlur(pos: vec2<f32>) -> vec4<f32> {
    let texSize: vec2<i32> = vec2<i32>(textureDimensions(boundTexture_t));

    let sigma: f32 = constants.blur_radius;
    let half_size: i32 = i32(ceil(sigma * 3.0));
    let limit: vec2<i32> = texSize - 1;
    let max_square: i32 = half_size * half_size;
    let g1: f32 = 1.0 / (2. * 3.141592 * sigma * sigma);
    let g2: f32 = -0.5 / (sigma * sigma);
    let w: vec2<f32> = 1.0 / vec2<f32>(texSize);
    let lo = vec2<f32>(0.5) / vec2<f32>(texSize);
    let hi = vec2<f32>(vec2<f32>(texSize) - 0.5) / vec2<f32>(texSize);
    var sum: vec4<f32> = g1 * textureSample(boundTexture_t, boundTexture_s, pos);

    for (var i: i32 = 1; i <= half_size; i = i + 2) {
        for (var j: i32 = 0; j <= half_size; j = j + 2) {
            let abcd = g1 * exp((sqr4(vec2<f32>(f32(i), f32(i + 1)).xyxy) + sqr4(vec2<f32>(f32(j), f32(j + 1)).xxyy)) * g2);

            let ab_sum = abcd[0] + abcd[1];
            let cd_sum = abcd[2] + abcd[3];
            let abcd_sum = ab_sum + cd_sum;

            let ab_y = abcd[1] / ab_sum;
            let cd_y = abcd[3] / cd_sum;
            let abcd_y = cd_sum / abcd_sum;

            let o = vec2<f32>((1.0 - abcd_y) * ab_y + abcd_y * cd_y, abcd_y);

            let xy = vec2<f32>(f32(i), f32(j)) + o;

            let v1 = textureSample(boundTexture_t, boundTexture_s, clamp(pos + w * xy, lo, hi));
            let v2 = textureSample(boundTexture_t, boundTexture_s, clamp(pos + w * conj(xy).yx, lo, hi));
            let v3 = textureSample(boundTexture_t, boundTexture_s, clamp(pos + w * -xy, lo, hi));
            let v4 = textureSample(boundTexture_t, boundTexture_s, clamp(pos + w * conj(xy.yx), lo, hi));
            sum = sum + (v1 + v2 + v3 + v4) * abcd_sum;
        }
    }
    return sum;
}

fn computeShadeColor(canvas_coord: vec2<f32>) -> vec4<f32> {
    var result: vec4<f32>;
    if constants.texture_index == -1 {
        let grad_pos = gradientPosition(canvas_coord);
        if constants.multigradient == -1 {
            result = simpleGradient(grad_pos);
        } else {
            result = multiGradient(grad_pos);
        }
        return result;
    }
    let transformed_uv = transformedTexCoord(canvas_coord);
    if constants.blur_radius > 0.0 {
        result = sampleBlur(transformed_uv);
    } else {
        result = textureSample(boundTexture_t, boundTexture_s, transformed_uv);
    }
    result = clamp(result, vec4<f32>(0.0), vec4<f32>(1.0));
    if constants.multigradient != -1 {
        result = multiGradient(result[constants.texture_channel]);
    }
    return result;
}

fn positionAlongLine(from_: vec2<f32>, to: vec2<f32>, point: vec2<f32>) -> f32 {
    let dir = normalize(to - from_);
    let offs = point - from_;
    return dot(offs, dir) / length(to - from_);
}

fn getAngle(x: vec2<f32>) -> f32 {
    return atan2(x.y, -x.x) / (2.0 * PI) + 0.5;
}

fn mapLine(from_: vec2<f32>, to: vec2<f32>, point: vec2<f32>) -> vec3<f32> {
    let len = length(to - from_);
    let dir = normalize(to - from_);
    return vec3<f32>(map(point - from_, dir), len);
}

fn sdfInfLine(p: vec2<f32>, p1: vec2<f32>, p2: vec2<f32>) -> f32 {
    let dir: vec2<f32> = normalize(p2 - p1);
    let normal: vec2<f32> = vec2<f32>(-dir.y, dir.x);
    return dot(p - p1, normal);
}

fn angleGradient(p: vec2<f32>, p1: vec2<f32>, p2: vec2<f32>) -> f32 {
    let sd = sdfInfLine(p, p1, p2);
    let n = map(normalize(p - p1), normalize(p2 - p1));
    let first = atan2(n.x, n.y) * 0.15915494309 + 0.75;
    let second = atan2(-n.x, -n.y) * 0.15915494309 + 0.25;
    return mix(first, second, clamp(sd + 0.5, 0., 1.));
}

fn gradientPositionForPoint(point: vec2<f32>) -> f32 {
    if constants.gradient == gradient_linear {
        return positionAlongLine(constants.gradient_point1, constants.gradient_point2, point);
    } else if constants.gradient == gradient_radial {
        return length(point - constants.gradient_point1) / length(constants.gradient_point2 - constants.gradient_point1);
    } else if constants.gradient == gradient_angle {
        return angleGradient(point, constants.gradient_point1, constants.gradient_point2);
    } else if constants.gradient == gradient_reflected {
        return 1.0 - abs(fract(positionAlongLine(constants.gradient_point1, constants.gradient_point2, point)) * 2.0 - 1.0);
    } else {
        return 0.5;
    }
}

fn gradientPosition(canvas_coord: vec2<f32>) -> f32 {
    let pos = gradientPositionForPoint(canvas_coord);
    return clamp(pos, 0.0, 1.0);
}

fn samplePattern(x: u32, pattern: u32) -> u32 {
    return (pattern >> (x % 12u)) & 1u;
}

fn atlas(sprite: i32, pos: vec2<i32>, stride: u32) -> f32 {
    if pos.x < 0 || pos.x >= i32(stride) {
        return 0.;
    }
    if sprite < 0 {
        return f32(u32(pos.x & pos.y) & 1u);
    }
    let linear: u32 = u32(sprite) * atlasAlignment + u32(pos.x) + u32(pos.y) * stride;
    return textureLoad(fontTex_t, vec2<u32>(linear % perFrame.atlas_width, linear / perFrame.atlas_width), 0u).r;
}

fn atlasRGBA(sprite: i32, pos: vec2<i32>, stride: u32) -> vec4<f32> {
    if pos.x < 0 || pos.x * 4 + 3 >= i32(stride) {
        return vec4<f32>(0.);
    }
    if sprite < 0 {
        return vec4<f32>(f32(u32(pos.x & pos.y) & 1u));
    }
    let linear: u32 = u32(sprite) * atlasAlignment + u32(pos.x) * 4u + u32(pos.y) * stride;
    let c = vec2<u32>(linear % perFrame.atlas_width, linear / perFrame.atlas_width);
    return vec4<f32>(
        textureLoad(fontTex_t, c, 0u).r,
        textureLoad(fontTex_t, c + vec2<u32>(1u, 0u), 0u).r,
        textureLoad(fontTex_t, c + vec2<u32>(2u, 0u), 0u).r,
        textureLoad(fontTex_t, c + vec2<u32>(3u, 0u), 0u).r
    );
}

fn atlasAccum(sprite: i32, pos: vec2<i32>, stride: u32) -> f32 {
    var alpha: f32 = 0.;
    if constants.sprite_oversampling == 1 {
        alpha = atlas(sprite, pos, stride);
        return alpha;
    }
    for (var i = 0; i < constants.sprite_oversampling; i++) {
        alpha += atlas(sprite, pos + vec2<i32>(i, 0), stride);
    }
    return alpha / f32(constants.sprite_oversampling);
}

fn processSubpixelOutput(rgb: vec3<f32>) -> vec3<f32> {
    if constants.subpixel == subpixel_off {
        return vec3<f32>(dot(rgb, vec3<f32>(0.3333, 0.3334, 0.3333)));
    } else if constants.subpixel == subpixel_bgr {
        return rgb.bgr;
    } else {
        return rgb;
    }
}

fn atlasSubpixel(sprite: i32, pos: vec2<i32>, stride: u32) -> vec3<f32> {
    if constants.sprite_oversampling == 6 {
        let x0 = atlas(sprite, pos + vec2<i32>(-2, 0), stride) + atlas(sprite, pos + vec2<i32>(-1, 0), stride);
        let x1 = atlas(sprite, pos + vec2<i32>(0, 0), stride) + atlas(sprite, pos + vec2<i32>(1, 0), stride);
        let x2 = atlas(sprite, pos + vec2<i32>(2, 0), stride) + atlas(sprite, pos + vec2<i32>(3, 0), stride);
        let x3 = atlas(sprite, pos + vec2<i32>(4, 0), stride) + atlas(sprite, pos + vec2<i32>(5, 0), stride);
        let x4 = atlas(sprite, pos + vec2<i32>(6, 0), stride) + atlas(sprite, pos + vec2<i32>(7, 0), stride);
        let filt = vec3<f32>(0.25, 0.5, 0.25) * 0.5;
        return vec3<f32>(dot(vec3<f32>(x0, x1, x2), filt), dot(vec3<f32>(x1, x2, x3), filt), dot(vec3<f32>(x2, x3, x4), filt));
    } else if constants.sprite_oversampling == 3 {
        let x0 = atlas(sprite, pos + vec2<i32>(-2, 0), stride);
        let x1 = atlas(sprite, pos + vec2<i32>(-1, 0), stride);
        let x2 = atlas(sprite, pos + vec2<i32>(0, 0), stride);
        let x3 = atlas(sprite, pos + vec2<i32>(1, 0), stride);
        let x4 = atlas(sprite, pos + vec2<i32>(2, 0), stride);
        let x5 = atlas(sprite, pos + vec2<i32>(3, 0), stride);
        let x6 = atlas(sprite, pos + vec2<i32>(4, 0), stride);
        const filt = array<f32, 3>(0x08 / 256.0, 0x4D / 256.0, 0x56 / 256.0);
        return vec3<f32>(
            x0 * filt[0] + x1 * filt[1] + x2 * filt[2] + x3 * filt[1] + x4 * filt[0],
            x1 * filt[0] + x2 * filt[1] + x3 * filt[2] + x4 * filt[1] + x5 * filt[0],
            x2 * filt[0] + x3 * filt[1] + x4 * filt[2] + x5 * filt[1] + x6 * filt[0]
        );
    } else {
        return vec3<f32>(1.);
    }
}

// Code by Evan Wallace, CC0 license 

fn gaussian(x: f32, sigma: f32) -> f32 {
    return exp(-((x * x) / (2.0 * sigma * sigma))) / (sqrt(2.0 * PI) * sigma);
}

// This approximates the error function, needed for the gaussian integral
fn erf(x: vec2<f32>) -> vec2<f32> {
    let s: vec2<f32> = sign(x);
    let a: vec2<f32> = abs(x);
    let x1 = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
    let x2 = x1 * x1;
    return s - s / (x2 * x2);
}

// Return the blurred mask along the x dimension
fn roundedBoxShadowX(x: f32, y: f32, sigma: f32, corner: f32, halfSize: vec2<f32>) -> f32 {
    let delta: f32 = min(halfSize.y - corner - abs(y), 0.0);
    let curved: f32 = halfSize.x - corner + sqrt(max(0.0, corner * corner - delta * delta));
    let integral: vec2<f32> = 0.5 + 0.5 * erf((vec2<f32>(x) + vec2<f32>(-curved, curved)) * (sqrt(0.5) / sigma));
    return integral.y - integral.x;
}

// Return the mask for the shadow of a box
fn roundedBoxShadow(halfSize: vec2<f32>, point: vec2<f32>, sigma: f32, border_radii: vec4<f32>) -> f32 {
    // The signal is only non-zero in a limited range, so don't waste samples
    let low: f32 = point.y - halfSize.y;
    let high: f32 = point.y + halfSize.y;
    let start: f32 = clamp(-3.0 * sigma, low, high);
    let end: f32 = clamp(3.0 * sigma, low, high);

    // Accumulate samples (we can get away with surprisingly few samples)
    let step: f32 = (end - start) / 4.0;
    var y: f32 = start + step * 0.5;
    var value: f32 = 0.0;

    let quadrant = u32(point.x >= 0.) + 2u * u32(point.y >= 0.);
    let corner: f32 = abs(border_radii[quadrant]);

    for (var i: i32 = 0; i < 4; i = i + 1) {
        value = value + roundedBoxShadowX(point.x, point.y - y, sigma, corner, halfSize) * gaussian(y, sigma) * step;
        y = y + step;
    }

    return value;
}

// End of code by Evan Wallace, CC0 license

fn applyGamma(in: vec4<f32>, gamma: f32) -> vec4<f32> {
    return pow(max(in, vec4<f32>(0.)), vec4<f32>(gamma));
}

fn applyBlueLightFilter(in: vec4<f32>, intensity: f32) -> vec4<f32> {
    return in * vec4<f32>(1.0, 1.0 - intensity * 0.6 * 0.6, 1.0 - intensity * 0.6, 1.0);
}

struct FragOut {
    @location(0) @blend_src(0) color: vec4<f32>,
    @location(0) @blend_src(1) blend: vec4<f32>,
}

fn useBlending() -> bool {
    return (constants.shader == shader_text || constants.shader == shader_mask) && constants.subpixel != subpixel_off;
}

fn postprocessColor(in: FragOut, canvas_coord: vec2<u32>) -> FragOut {
    var out: FragOut = in;
    var opacity = constants.opacity;

    if constants.pattern != 0u {
        let pattern_scale = constants.pattern >> 24u;
        let hpattern = constants.pattern & 0xFFFu;
        let vpattern = constants.pattern >> 12u;
        let p: u32 = samplePattern(canvas_coord.x / pattern_scale, hpattern) & samplePattern(canvas_coord.y / pattern_scale, vpattern);
        opacity = opacity * f32(p);
    }
    out.color = out.color * opacity;
    if useBlending() {
        out.blend = out.blend * opacity;
    }

    if perFrame.blue_light_filter != 0. {
        out.color = applyBlueLightFilter(out.color, perFrame.blue_light_filter);
        if useBlending() {
            out.blend = applyBlueLightFilter(out.blend, perFrame.blue_light_filter);
        }
    }
    if perFrame.global_gamma != 1. {
        out.color = applyGamma(out.color, perFrame.global_gamma);
        if useBlending() {
            out.blend = applyGamma(out.blend, perFrame.global_gamma);
        }
    }
    if !useBlending() {
        out.blend = vec4<f32>(out.color.a);
    }

    return out;
}

fn rectangleCoverage(pt: vec2<f32>, rect: vec4<f32>) -> f32 {
    let wh = max(vec2<f32>(0.0), min(pt.xy + vec2<f32>(0.5), rect.zw) - max(pt.xy - vec2<f32>(0.5), rect.xy));

    return wh.x * wh.y;
}

@fragment /**/fn fragmentMain(in: VertexOutput) -> FragOut {

    if constants.shader == shader_blit {
        let tex_coord = vec2<i32>(in.position.xy);
        return FragOut(textureLoad(boundTexture_t, tex_coord, 0), vec4<f32>(1.));
    }

    var outColor: vec4<f32>;
    var outBlend: vec4<f32>;

    if constants.shader == shader_shadow {
        outColor = constants.fill_color1 * (roundedBoxShadow(in.data0.xy * 0.5, in.uv, constants.blur_radius, in.data1));
    } else if constants.shader == shader_color_mask || constants.shader == shader_text {
        let sprite = i32(in.data0.z);
        let stride = u32(in.data0.w);
        let tuv = vec2<i32>(in.uv);
        let shadeColor: vec4<f32> = computeShadeColor(in.canvas_coord);

        if useBlending() {
            let rgb = processSubpixelOutput(atlasSubpixel(sprite, tuv, stride));
            outColor = shadeColor * vec4<f32>(rgb, 1.);
            outBlend = vec4<f32>(shadeColor.a * rgb, 1.);
        } else if constants.shader == shader_color_mask {
            outColor = shadeColor * atlasRGBA(sprite, tuv, stride);
        } else {
            var alpha = atlasAccum(sprite, tuv, stride);
            outColor = shadeColor * vec4<f32>(alpha);
        }
    } else if constants.shader == shader_mask {
        let xy: vec2<u32> = vec2<u32>(in.uv);
        let cov: f32 = unpack4x8unorm(in.coverage[xy.y & 3u])[xy.x & 3u];
        let shadeColor: vec4<f32> = computeShadeColor(in.canvas_coord);
        outColor = shadeColor * vec4<f32>(cov);
    } else if constants.shader == shader_rectangle {
        let shadeColor: vec4<f32> = computeShadeColor(in.canvas_coord);
        let rect = in.data0;
        let pixelCoverage = rectangleCoverage(in.canvas_coord, rect);

        outColor = pixelCoverage * shadeColor;
    } else {
        outColor = vec4<f32>(0., 1., 0., 0.5);
    }

    return postprocessColor(FragOut(outColor, outBlend), vec2<u32>(in.canvas_coord));
}
