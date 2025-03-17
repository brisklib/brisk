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
    @builtin(position) position: vec4<f32>,
    @location(0) @interpolate(linear) data0: vec4<f32>,
    @location(1) @interpolate(linear) data1: vec4<f32>,
    @location(2) @interpolate(linear, center) uv: vec2<f32>,
    @location(3) @interpolate(linear, center) canvas_coord: vec2<f32>,
};

const PI = 3.1415926535897932384626433832795;

const atlasAlignment     = 8u;

const shader_rectangles  = shader_type(0);
const shader_arcs        = shader_type(1);
const shader_text        = shader_type(2);
const shader_shadow      = shader_type(3);
const shader_mask        = shader_type(4);
const shader_color_mask  = shader_type(5);

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
    
    scissorPoint1: vec2<f32>,
    scissorPoint2: vec2<f32>,
    scissorPoint3: vec2<f32>,

    coord_matrix_a: f32,
    coord_matrix_b: f32,
    coord_matrix_c: f32,
    coord_matrix_d: f32,

    coord_matrix_e: f32,
    coord_matrix_f: f32,
    sprite_oversampling: i32,
    subpixel: subpixel_mode,

    pattern: u32,
    reserved_1: i32,
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

    stroke_color1: vec4<f32>,

    stroke_color2: vec4<f32>,

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

fn distanceScale() -> f32 {
    let ac = vec2<f32>(constants.coord_matrix_a, constants.coord_matrix_c);
    return sqrt(dot(ac, ac));
}

@group(0) @binding(1) var<uniform> constants: UniformBlock;
@group(0) @binding(2) var<uniform> perFrame: UniformBlockPerFrame;

@group(0) @binding(3) var<storage, read> data: array<vec4<f32>>;

@group(0) @binding(8) var gradTex_t: texture_2d<f32>;

@group(0) @binding(9) var fontTex_t: texture_2d<f32>;

@group(0) @binding(10) var boundTexture_t: texture_2d<f32>;

@group(0) @binding(6) var boundTexture_s: sampler;

@group(0) @binding(7) var gradTex_s: sampler;

fn to_screen(xy: vec2<f32>) -> vec2<f32> {
    return xy * perFrame.viewport.zw * vec2<f32>(2., -2.) + vec2<f32>(-1., 1.);
}

fn from_screen(xy: vec2<f32>) -> vec2<f32> {
    return (xy - vec2<f32>(-1., 1.)) * vec2<f32>(0.5, -0.5) * perFrame.viewport.xy;
}

fn max2(pt: vec2<f32>) -> f32 {
    return max(pt.x, pt.y);
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

fn scissorTest(p: vec2<f32>) -> bool {
    let scissorPoint4 = constants.scissorPoint1 + (constants.scissorPoint3 - constants.scissorPoint2);
    return isPointInQuad(p, constants.scissorPoint1, constants.scissorPoint2, constants.scissorPoint3, scissorPoint4);
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

@vertex /**/fn vertexMain(@builtin(vertex_index) vidx: u32, @builtin(instance_index) inst: u32) -> VertexOutput {
    const vertices = array(vec2<f32>(-0.5, -0.5), vec2<f32>(0.5, -0.5), vec2<f32>(-0.5, 0.5), vec2<f32>(0.5, 0.5));
    var output: VertexOutput;

    let position = vertices[vidx];
    let uv_coord: vec2<f32> = position + vec2<f32>(0.5);
    var outPosition = vec4<f32>(0.);
    if constants.shader == shader_rectangles || constants.shader == shader_shadow {
        let m = margin();
        let rect = norm_rect(data[constants.data_offset + inst * 2u]);
        output.data0 = vec4<f32>(rect.zw - rect.xy, 0., 0.);
        let radii = data[constants.data_offset + inst * 2u + 1u];
        output.data1 = radii;
        let center = (rect.xy + rect.zw) * 0.5;
        let pt = mix(rect.xy - vec2<f32>(m), rect.zw + vec2<f32>(m), uv_coord);
        outPosition = vec4<f32>(pt, 0., 1.);
        output.uv = position * (m + m + rect.zw - rect.xy);
    } else if constants.shader == shader_arcs {
        let m = margin();
        let dat0 = data[constants.data_offset + inst * 2u];
        output.data0 = dat0;
        let dat1 = data[constants.data_offset + inst * 2u + 1u];
        output.data1 = dat1;

        outPosition = vec4<f32>(mix(dat0.xy - vec2<f32>(dat0.z + m), dat0.xy + vec2<f32>(dat0.z + m), uv_coord), 0., 1.);
        output.uv = position * (m + m + 2.0 * dat0.z);
    } else if constants.shader == shader_text {
        var rect = norm_rect(data[constants.data_offset + inst * 2u]);
        let glyph_data = data[constants.data_offset + inst * 2u + 1u];

        let base = rect.x;

        rect.x += perFrame.text_rect_offset;
        rect.z += perFrame.text_rect_offset;

        rect.x -= perFrame.text_rect_padding;
        rect.z += perFrame.text_rect_padding;

        outPosition = vec4<f32>(mix(rect.xy, rect.zw, uv_coord), 0., 1.);
        output.uv = (outPosition.xy - vec2<f32>(base, rect.y) + vec2<f32>(-perFrame.text_rect_padding, 0.)) * vec2<f32>(f32(constants.sprite_oversampling), 1.);
        output.data0 = glyph_data;
    } else if constants.shader == shader_mask || constants.shader == shader_color_mask {
        let rect = norm_rect(data[constants.data_offset + inst * 2u]);
        let glyph_data = data[constants.data_offset + inst * 2u + 1u];
        outPosition = vec4<f32>(mix(rect.xy, rect.zw, uv_coord), 0., 1.);
        output.uv = outPosition.xy - rect.xy;
        output.data0 = glyph_data;
    }

    output.canvas_coord = outPosition.xy;
    output.position = vec4<f32>(to_screen(transform2D(outPosition.xy)), outPosition.zw);
    return output;
}

fn superLength(pt: vec2<f32>) -> f32 {
    let p = 4.0;
    return pow(pow(pt.x, p) + pow(pt.y, p), 1.0 / p);
}

fn sdLengthEx(pt: vec2<f32>, border_radius: f32) -> f32 {
    if border_radius == 0.0 {
        return max2(abs(pt));
    } else if border_radius > 0.0 {
        return length(pt);
    } else {
        return superLength(pt);
    }
}

fn signedDistanceRectangle(pt: vec2<f32>, rect_size: vec2<f32>, border_radii: vec4<f32>) -> f32 {
    let ext = rect_size * 0.5;
    let quadrant = u32(pt.x >= 0.) + 2u * u32(pt.y >= 0.);
    var rad: f32 = abs(border_radii[quadrant]);
    let ext2 = ext - vec2(rad, rad);
    let d: vec2<f32> = abs(pt) - ext2;
    return (min(max(d.x, d.y), 0.0) + sdLengthEx(max(d, vec2<f32>(0.)), border_radii[quadrant]) - rad);
}

fn signedDistanceArc(pt: vec2<f32>, outer_radius: f32, inner_radius: f32, start_angle: f32, end_angle: f32) -> f32 {
    let outer_d = length(pt) - outer_radius;
    let inner_d = length(pt) - inner_radius;
    var circle = max(outer_d, -inner_d);

    if end_angle - start_angle < 2.0 * 3.141592653589793238 {
        let start_sincos = -vec2<f32>(cos(start_angle), sin(start_angle));
        let end_sincos = vec2<f32>(cos(end_angle), sin(end_angle));
        var pie: f32;
        let add = vec2<f32>(dot(pt, start_sincos), dot(pt, end_sincos));
        if end_angle - start_angle > 3.141592653589793238 {
            pie = min(add.x, add.y); // union
        } else {
            pie = max(add.x, add.y); // intersect
        }
        circle = max(circle, pie);
    }
    return circle;
}

struct Colors {
    brush: vec4<f32>,
    stroke: vec4<f32>,
}

fn simpleGradient(pos: f32, stroke: bool) -> vec4<f32> {
    let color1 = select(constants.fill_color1, constants.stroke_color1, stroke);
    let color2 = select(constants.fill_color2, constants.stroke_color2, stroke);
    return mix(color1, color2, pos);
}

fn multiGradient(pos: f32) -> vec4<f32> {
    let invDims = vec2<f32>(1.) / vec2<f32>(textureDimensions(gradTex_t));
    return textureSample(gradTex_t, gradTex_s,
        vec2<f32>(0.5 + pos * f32(gradientResolution - 1), 0.5 + f32(constants.multigradient)) * invDims);
}

fn remixColors(value: vec4<f32>) -> vec4<f32> {
    return constants.fill_color1 * value.x + constants.fill_color2 * value.y + constants.stroke_color1 * value.z + constants.stroke_color2 * value.w;
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

fn simpleCalcColors(canvas_coord: vec2<f32>) -> Colors {
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

fn sampleBlur(pos: vec2<f32>) -> vec4<f32> {
    let texSize: vec2<u32> = textureDimensions(boundTexture_t);

    if constants.blur_directions == 3u {
        let scale: vec2<f32> = vec2<f32>(1.0, 1.0) / vec2<f32>(texSize);
        switch (i32(round(constants.blur_radius * 2.0))) {
            case 0, 1: {
                // sigma = 0.1667 size=3x3
                return textureSample(boundTexture_t, boundTexture_s, pos);
            }
            case 2: {
                // sigma = 0.3333 size=3x3
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.010987, -0.010987)) * 0.97838 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.010987, 1.0)) * 0.010749 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.0, -0.010987)) * 0.010749 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.0, 1.0)) * 0.0001181;
            }
            case 3: {
                // sigma = 0.5000 size=5x5
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.0025, -1.0025)) * 0.011388 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.0025, 0.1192)) * 0.095298 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.1192, -1.0025)) * 0.095298 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.1192, 0.1192)) * 0.79749 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.1192, 2.0)) * 0.00023565 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.0, 0.1192)) * 0.00023565;
            }
            case 4: {
                // sigma = 0.6667 size=5x5
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.0331, -1.0331)) * 0.040349 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.0331, 0.24509)) * 0.15919 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.0331, 2.0036)) * 0.0013398 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.24509, -1.0331)) * 0.15919 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.24509, 0.24509)) * 0.62803 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.24509, 2.0036)) * 0.0052859 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.0036, -1.0331)) * 0.0013398 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.0036, 0.24509)) * 0.0052859;
            }
            case 5: {
                // sigma = 0.8333 size=7x7
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.0266, -2.0266)) * 0.0007622 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.0266, -0.32739)) * 0.01965 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.0266, 1.1034)) * 0.0071753 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.32739, -2.0266)) * 0.01965 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.32739, -0.32739)) * 0.5066 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.32739, 1.1034)) * 0.18499 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.32739, 3.0064)) * 0.00052602 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.1034, -2.0266)) * 0.0071753 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.1034, -0.32739)) * 0.18499 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.1034, 1.1034)) * 0.067547 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.1034, 3.0064)) * 0.00019208 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.0064, -0.32739)) * 0.00052602 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.0064, 1.1034)) * 0.00019208;
            }
            case 6: {
                // sigma = 1.0000 size=7x7
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.0759, -2.0759)) * 0.0034151 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.0759, -0.37754)) * 0.037464 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.0759, 1.1824)) * 0.0173 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.0759, 3.0293)) * 0.00026688 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.37754, -2.0759)) * 0.037464 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.37754, -0.37754)) * 0.41099 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.37754, 1.1824)) * 0.18979 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.37754, 3.0293)) * 0.0029278 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.1824, -2.0759)) * 0.0173 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.1824, -0.37754)) * 0.18979 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.1824, 1.1824)) * 0.087641 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.1824, 3.0293)) * 0.001352 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.0293, -2.0759)) * 0.00026688 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.0293, -0.37754)) * 0.0029278 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.0293, 1.1824)) * 0.001352;
            }
            case 7: {
                // sigma = 1.1667 size=9x9
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-3.071, -3.071)) * 0.00018209 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-3.071, -1.2494)) * 0.0042577 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-3.071, 0.40918)) * 0.0078107 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-3.071, 2.1374)) * 0.0012308 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.2494, -3.071)) * 0.0042577 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.2494, -1.2494)) * 0.099552 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.2494, 0.40918)) * 0.18263 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.2494, 2.1374)) * 0.028779 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.2494, 4.0354)) * 0.00031338 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.40918, -3.071)) * 0.0078107 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.40918, -1.2494)) * 0.18263 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.40918, 0.40918)) * 0.33503 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.40918, 2.1374)) * 0.052796 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.40918, 4.0354)) * 0.00057489 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1374, -3.071)) * 0.0012308 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1374, -1.2494)) * 0.028779 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1374, 0.40918)) * 0.052796 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1374, 2.1374)) * 0.0083198 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(4.0354, -1.2494)) * 0.00031338 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(4.0354, 0.40918)) * 0.00057489;
            }
            case 8: {
                // sigma = 1.3333 size=9x9
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-3.1225, -3.1225)) * 0.00073678 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-3.1225, -1.3007)) * 0.008772 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-3.1225, 0.43015)) * 0.01426 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-3.1225, 2.1968)) * 0.0032847 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.3007, -3.1225)) * 0.008772 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.3007, -1.3007)) * 0.10444 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.3007, 0.43015)) * 0.16978 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.3007, 2.1968)) * 0.039107 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-1.3007, 4.0737)) * 0.0011603 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.43015, -3.1225)) * 0.01426 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.43015, -1.3007)) * 0.16978 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.43015, 0.43015)) * 0.27599 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.43015, 2.1968)) * 0.063573 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(0.43015, 4.0737)) * 0.0018862 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1968, -3.1225)) * 0.0032847 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1968, -1.3007)) * 0.039107 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1968, 0.43015)) * 0.063573 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1968, 2.1968)) * 0.014643 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(2.1968, 4.0737)) * 0.00043446 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(4.0737, -1.3007)) * 0.0011603 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(4.0737, 0.43015)) * 0.0018862 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(4.0737, 2.1968)) * 0.00043446;
            }
            default: {
                // sigma = 1.5000 size=11x11
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-4.1192, -2.2477)) * 0.0012541 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-4.1192, -0.44467)) * 0.0041325 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-4.1192, 1.3392)) * 0.0027811 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-4.1192, 3.1743)) * 0.00037614 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.2477, -4.1192)) * 0.0012541 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.2477, -2.2477)) * 0.02113 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.2477, -0.44467)) * 0.069631 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.2477, 1.3392)) * 0.04686 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.2477, 3.1743)) * 0.0063377 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-2.2477, 5.0798)) * 0.00016246 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.44467, -4.1192)) * 0.0041325 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.44467, -2.2477)) * 0.069631 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.44467, -0.44467)) * 0.22946 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.44467, 1.3392)) * 0.15442 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.44467, 3.1743)) * 0.020885 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(-0.44467, 5.0798)) * 0.00053536 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.3392, -4.1192)) * 0.0027811 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.3392, -2.2477)) * 0.04686 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.3392, -0.44467)) * 0.15442 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.3392, 1.3392)) * 0.10392 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.3392, 3.1743)) * 0.014055 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(1.3392, 5.0798)) * 0.00036028 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.1743, -4.1192)) * 0.00037614 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.1743, -2.2477)) * 0.0063377 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.1743, -0.44467)) * 0.020885 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.1743, 1.3392)) * 0.014055 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(3.1743, 3.1743)) * 0.0019009 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(5.0798, -2.2477)) * 0.00016246 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(5.0798, -0.44467)) * 0.00053536 + textureSample(boundTexture_t, boundTexture_s, pos + scale * vec2<f32>(5.0798, 1.3392)) * 0.00036028;
            }
        }
    } else {
        let scale: vec2<f32> = select(vec2<f32>(0.0, 1.0), vec2<f32>(1.0, 0.0), constants.blur_directions == 1u) / vec2<f32>(texSize);
        switch (i32(round(constants.blur_radius * 2.0))) {
            case 0, 1: {
                // sigma = 0.1667 size=3x1
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * -1.523e-08) * 1.0;
            }
            case 2: {
                // sigma = 0.3333 size=3x1
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * -0.010987) * 0.98913 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 1.0) * 0.010868;
            }
            case 3: {
                // sigma = 0.5000 size=5x1
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * -1.0025) * 0.10671 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 0.1192) * 0.89302 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 2.0) * 0.00026388;
            }
            case 4: {
                // sigma = 0.6667 size=5x1
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * -1.0331) * 0.20087 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 0.24509) * 0.79248 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 2.0036) * 0.00667;
            }
            case 5: {
                // sigma = 0.8333 size=7x1
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * -2.0266) * 0.027608 + textureSample(boundTexture_t, boundTexture_s, pos + scale * -0.32739) * 0.71176 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 1.1034) * 0.2599 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 3.0064) * 0.00073904;
            }
            case 6: {
                // sigma = 1.0000 size=7x1
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * -2.0759) * 0.058439 + textureSample(boundTexture_t, boundTexture_s, pos + scale * -0.37754) * 0.64109 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 1.1824) * 0.29604 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 3.0293) * 0.0045669;
            }
            case 7: {
                // sigma = 1.1667 size=9x1
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * -3.071) * 0.013494 + textureSample(boundTexture_t, boundTexture_s, pos + scale * -1.2494) * 0.31552 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 0.40918) * 0.57882 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 2.1374) * 0.091213 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 4.0354) * 0.00099321;
            }
            case 8: {
                // sigma = 1.3333 size=9x1
                return textureSample(boundTexture_t, boundTexture_s, pos + scale * -3.1225) * 0.027144 + textureSample(boundTexture_t, boundTexture_s, pos + scale * -1.3007) * 0.32317 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 0.43015) * 0.52535 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 2.1968) * 0.12101 + textureSample(boundTexture_t, boundTexture_s, pos + scale * 4.0737) * 0.0035903;
            }
            default: {
                var sum: vec4<f32> = vec4<f32>(0.0);
                let s: f32 = constants.blur_radius * 0.33333333333333 / 2.0;
                let half_size: i32 = i32(s * 2.5);
                let g1: f32 = 1.0 / sqrt(2.0 * 3.1415926535897932384626433832795 * s * s);
                let g2: f32 = 1.0 / (2.0 * s * s);

                let ipos: vec2<i32> = vec2<i32>(pos * vec2<f32>(texSize));
                sum = textureLoad(boundTexture_t, ipos, 0) * g1;

                let step: vec2<i32> = select(vec2<i32>(0, 1), vec2<i32>(1, 0), constants.blur_directions == 1u);

                for (var i: i32 = 1; i <= half_size; i = i + 1) {
                    let g: f32 = g1 * exp(-f32(i * i) * g2);
                    sum = sum + (textureLoad(boundTexture_t, ipos + step * i, 0) + textureLoad(boundTexture_t, ipos - step * i, 0)) * g;
                }
                return sum;
            }
        }
    }
}

fn calcColors(canvas_coord: vec2<f32>) -> Colors {
    var result: Colors;
    if constants.texture_index != -1 { // texture
        let transformed_uv = transformedTexCoord(canvas_coord);
        if constants.blur_radius > 0.0 {
            result.brush = sampleBlur(transformed_uv);
        } else {
            result.brush = textureSample(boundTexture_t, boundTexture_s, transformed_uv);
        }
        result.brush = clamp(result.brush, vec4<f32>(0.0), vec4<f32>(1.0));
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

fn gradientPositionForPoint(point: vec2<f32>) -> f32 {
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

fn gradientPosition(canvas_coord: vec2<f32>) -> f32 {
    let pos = gradientPositionForPoint(canvas_coord);
    return clamp(pos, 0.0, 1.0);
}

fn toCoverage(sd: f32) -> f32 {
    return clamp(0.5 - sd * distanceScale(), 0.0, 1.0);
}

fn fillOnly(signed_distance: f32, colors: Colors) -> vec4<f32> {
    return colors.brush * toCoverage(signed_distance);
}

fn blendNormalPremultiplied(src: vec4<f32>, dst: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(src.rgb + dst.rgb * (1.0 - src.a), src.a + dst.a * (1.0 - src.a));
}

fn fillAndStroke(stroke_sd: f32, fill_sd: f32, colors: Colors) -> vec4<f32> {
    let backColor = colors.brush * toCoverage(fill_sd);
    let foreColor = colors.stroke * toCoverage(stroke_sd);
    return blendNormalPremultiplied(foreColor, backColor);
}

fn signedDistanceToColor(sd: f32, canvas_coord: vec2<f32>, uv: vec2<f32>, rectSize: vec2<f32>) -> vec4<f32> {
    let colors: Colors = calcColors(canvas_coord);
    if constants.stroke_width > 0.0 {
        let stroke_sd = abs(sd) - constants.stroke_width * 0.5;
        return fillAndStroke(stroke_sd, sd, colors);
    } else {
        return fillOnly(sd, colors);
    }
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
    let pi: f32 = 3.141592653589793;
    return exp(-((x * x) / (2.0 * sigma * sigma))) / (sqrt(2.0 * pi) * sigma);
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

@fragment /**/fn fragmentMain(in: VertexOutput) -> FragOut {

    if !scissorTest(in.position.xy) {
        discard;
    }
    var outColor: vec4<f32>;
    var outBlend: vec4<f32>;
    var useBlend: bool = false;

    if constants.shader == shader_rectangles || constants.shader == shader_arcs {
        var sd: f32;
        if constants.shader == shader_rectangles {
            sd = signedDistanceRectangle(in.uv, in.data0.xy, in.data1);
        } else { // shader_arcs
            sd = signedDistanceArc(in.uv, in.data0.z, in.data0.w, in.data1.x, in.data1.y);
        }
        outColor = signedDistanceToColor(sd, in.canvas_coord, in.uv, in.data0.xy);
    } else if constants.shader == shader_shadow {
        outColor = constants.fill_color1 * (roundedBoxShadow(in.data0.xy * 0.5, in.uv, constants.blur_radius, in.data1));
    } else if constants.shader == shader_mask || constants.shader == shader_color_mask || constants.shader == shader_text {
        let sprite = i32(in.data0.z);
        let stride = u32(in.data0.w);
        let tuv = vec2<i32>(in.uv);
        let colors: Colors = calcColors(in.canvas_coord);

        if useBlending() {
            let rgb = atlasSubpixel(sprite, tuv, stride);
            outColor = colors.brush * vec4<f32>(rgb, 1.);
            outBlend = vec4<f32>(colors.brush.a * rgb, 1.);
        } else if constants.shader == shader_color_mask {
            outColor = colors.brush * atlasRGBA(sprite, tuv, stride);
        } else {
            var alpha = atlasAccum(sprite, tuv, stride);
            outColor = colors.brush * vec4<f32>(alpha);
        }
    }

    return postprocessColor(FragOut(outColor, outBlend), vec2<u32>(in.canvas_coord));
}
