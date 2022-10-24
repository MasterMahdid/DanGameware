[[FX]]

// Samplers
sampler2D buf0 = sampler_state
{
	Address = Clamp;
};


// Uniforms
float hdrExposure = 2.0;       // Exposure (higher values make scene brighter)
float bloodMask = 0;


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

[[VS_FSQUAD]]
// =================================================================================================

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;
				
void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1.0 );
}


[[FS_BLUR]]
// =================================================================================================


uniform sampler2D buf0;
uniform vec2 frameBufSize;
varying vec2 texCoords;

uniform bool horizontal;

void main( void )
{
	
}
	

[[FS_FINALPASS]]
// =================================================================================================

uniform sampler2D buf0, buf1;
uniform vec2 frameBufSize;
uniform float hdrExposure;
uniform float bloodMask;
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
	vec4 col0 = texture2D( buf0, texCoords );
	vec3 fcol = ACESFitted(col0.rgb);
	fcol = pow(fcol, vec3(1.0 / 2.2));
	gl_FragColor.a = 1.0;
	gl_FragColor.rgb =fcol;
	vec3 bloodcol = gl_FragColor.rgb;
	bloodcol.g *= 0.0;
	bloodcol.b *= 0.0;
	bloodcol.r = clamp(bloodcol.r-0.1,0.0,1.0);

	gl_FragColor.rgb = mix(gl_FragColor.rgb,bloodcol,bloodMask);

}

