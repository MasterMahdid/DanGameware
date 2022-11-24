[[FX]]

// Samplers
sampler2D buf0 = sampler_state
{
	Address = Clamp;
};

sampler2D buf1 = sampler_state
{
	Address = Clamp;
};

// Uniforms
float hdrExposure = 2.0;       // Exposure (higher values make scene brighter)
float hdrBrightThres = 0.6;    // Brightpass threshold (intensity where blooming begins)
float hdrBrightOffset = 0.06;  // Brightpass offset (smaller values produce stronger blooming)

float4 blurParams = {0, 0, 0, 0};

// Contexts
context BRIGHTPASS
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_BRIGHTPASS;
	
	ZWriteEnable = false;
}

context BLUR
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_BLUR;
	
	ZWriteEnable = false;
}

context FINALPASS
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_FINALPASS;
	
	ZWriteEnable = false;
}

OpenGL4
{
	context BRIGHTPASS
	{
		VertexShader = compile GLSL VS_FSQUAD_GL4;
		PixelShader = compile GLSL FS_BRIGHTPASS_GL4;
		
		ZWriteEnable = false;
	}

	context BLUR
	{
		VertexShader = compile GLSL VS_FSQUAD_GL4;
		PixelShader = compile GLSL FS_BLUR_GL4;
		
		ZWriteEnable = false;
	}

	context FINALPASS
	{
		VertexShader = compile GLSL VS_FSQUAD_GL4;
		PixelShader = compile GLSL FS_FINALPASS_GL4;
		
		ZWriteEnable = false;
	}
}

[[VS_FSQUAD]]
// =================================================================================================

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;
				
void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1 );
}

[[VS_FSQUAD_GL4]]
// =================================================================================================

uniform mat4 projMat;

layout( location = 0 ) in vec3 vertPos;
out vec2 texCoords;
				
void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1 );
}


[[FS_BRIGHTPASS]]
// =================================================================================================

#include "shaders/utilityLib/fragPostProcess.glsl"

uniform sampler2D buf0;
uniform vec2 frameBufSize;
//uniform float hdrExposure;
uniform float hdrBrightThres;
uniform float hdrBrightOffset;
varying vec2 texCoords;

void main( void )
{
	vec2 texSize = frameBufSize * 4.0;
	vec2 coord2 = texCoords + vec2( 2, 2 ) / texSize;
	
	// Average using bilinear filtering
	vec4 sum = getTex2DBilinear( buf0, texCoords, texSize );
	sum += getTex2DBilinear( buf0, coord2, texSize );
	sum += getTex2DBilinear( buf0, vec2( coord2.x, texCoords.y ), texSize );
	sum += getTex2DBilinear( buf0, vec2( texCoords.x, coord2.y ), texSize );
	sum /= 4.0;
	
	// Tonemap
	//sum = 1.0 - exp2( -hdrExposure * sum );
	
	// Extract bright values
	sum = max( sum - hdrBrightThres, 0.0 );
	sum /= hdrBrightOffset + sum;
	
	gl_FragColor = sum;
}

[[FS_BRIGHTPASS_GL4]]
// =================================================================================================

#include "shaders/utilityLib/fragPostProcessGL4.glsl"

uniform sampler2D buf0;
uniform vec2 frameBufSize;
//uniform float hdrExposure;
uniform float hdrBrightThres;
uniform float hdrBrightOffset;
in vec2 texCoords;

out vec4 fragColor;

void main( void )
{
	vec2 texSize = frameBufSize * 4.0;
	vec2 coord2 = texCoords + vec2( 2, 2 ) / texSize;
	
	// Average using bilinear filtering
	vec4 sum = getTex2DBilinear( buf0, texCoords, texSize );
	sum += getTex2DBilinear( buf0, coord2, texSize );
	sum += getTex2DBilinear( buf0, vec2( coord2.x, texCoords.y ), texSize );
	sum += getTex2DBilinear( buf0, vec2( texCoords.x, coord2.y ), texSize );
	sum /= 4.0;
	
	// Tonemap
	//sum = 1.0 - exp2( -hdrExposure * sum );
	
	// Extract bright values
	sum = max( sum - hdrBrightThres, 0.0 );
	sum /= hdrBrightOffset + sum;
	
	fragColor = sum;
}

	
[[FS_BLUR]]
// =================================================================================================

#include "shaders/utilityLib/fragPostProcess.glsl"

uniform sampler2D buf0;
uniform vec2 frameBufSize;
uniform vec4 blurParams;
varying vec2 texCoords;

void main( void )
{
	gl_FragColor = blurKawase( buf0, texCoords, frameBufSize, blurParams.x );
}
	
[[FS_BLUR_GL4]]
// =================================================================================================

#include "shaders/utilityLib/fragPostProcessGL4.glsl"

uniform sampler2D buf0;
uniform vec2 frameBufSize;
uniform vec4 blurParams;
in vec2 texCoords;

out vec4 fragColor;

void main( void )
{
	fragColor = blurKawase( buf0, texCoords, frameBufSize, blurParams.x );
}


[[FS_FINALPASS]]
// =================================================================================================

uniform sampler2D buf0, buf1;
uniform vec2 frameBufSize;
uniform float hdrExposure;
varying vec2 texCoords;


vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.0,1.0);
}
// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
mat3 ACESInputMat =mat3(

    0.59719,0.07600,0.02840,
	0.35458,0.90834,0.13383,
	0.04823,0.01566,0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB

mat3 ACESOutputMat = mat3(
	1.60475,-0.10208,-0.00327,
	-0.53108,1.10813,-0.07276,
	-0.07367,-0.00605, 1.07602 
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = ACESInputMat*color;
    color = RRTAndODTFit(color);
    color = ACESOutputMat*color;
    color = clamp(color,0.0,1.0);
    return color;
}

void main( void )
{
	vec4 col0 = texture2D( buf0, texCoords );	// HDR color
	vec4 col1 = texture2D( buf1, texCoords );	// Bloom
	col0 = 1.0 - exp2( -hdrExposure * col0 );
	col0.rgb = ACESFitted(col0.rgb);
	col1.rgb = ACESFitted(col1.rgb);
	
	// Tonemap (using photographic exposure mapping)
	
	gl_FragColor = col0 + col1;

	//gl_FragColor = col1;//to see only the blur
	gl_FragColor.rgb =  pow(gl_FragColor.rgb, vec3(1.0 / 2.2));
}

[[FS_FINALPASS_GL4]]
// =================================================================================================

uniform sampler2D buf0, buf1;
uniform vec2 frameBufSize;
uniform float hdrExposure;
in vec2 texCoords;

out vec4 fragColor;

void main( void )
{
	vec4 col0 = texture( buf0, texCoords );	// HDR color
	vec4 col1 = texture( buf1, texCoords );	// Bloom
	
	// Tonemap (using photographic exposure mapping)
	vec4 col = 1.0 - exp2( -hdrExposure * col0 );
	
	fragColor = col + col1;
}