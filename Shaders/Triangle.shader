Shader "Triangle"
{
	Properties
	{
		_Color("Main Color", Color) = (1, 1, 1, 1)
	}

	SubShader
	{
		Cull Off
		ZWrite On
		ZTest Less
		Blend Opaque

		Pass "BasePass"
		{
			#pragma vertex vert_main
			#pragma fragment frag_main

			#version 450
			#extension GL_GOOGLE_include_directive : enable
			#include "MahoCommon.glsl"

			// ═══════════════════════════════════════
			//  Vertex input (a2v)
			//  Each member MUST carry a semantic;
			//  engine expands into layout(location=N) in.
			// ═══════════════════════════════════════

			struct a2v
			{
				vec3 position : POSITION;   // loc=0  model‑space vertex position
				vec3 color    : COLOR0;     // loc=5  per‑vertex color
				// ── commented‑out templates ──
				// vec3 normal    : NORMAL;       // loc=1
				// vec2 uv0       : TEXCOORD0;    // loc=2
				// vec2 uv1       : TEXCOORD1;    // loc=3  lightmap / mask
				// vec4 tangent   : TANGENT;      // loc=4  .xyz tangent, .w bitangent sign
				// uvec4 boneIdx  : BONEINDICES;  // loc=13
				// vec4  boneWgt  : BONEWEIGHTS;  // loc=14
			};

			// ── GLSL built‑in input globals (no semantic needed) ──
			//
			//   gl_VertexIndex  int   current vertex index (0‑based)
			//   gl_InstanceIndex int   current instance ID (GPU‑driven)
			//   gl_DrawID       int   current draw ID (multi‑draw indirect)
			//   gl_BaseVertex   int   first vertex offset in draw call
			//   gl_BaseInstance int   first instance offset in draw call

			// ═══════════════════════════════════════
			//  Inter‑stage (v2f)
			//  Written by vert_main, read by frag_main.
			//  Each member MUST carry a semantic;
			//  engine expands into layout(location=N).
			// ═══════════════════════════════════════

			struct v2f
			{
				// ── mandatory ──
				vec4 clipPos : SV_POSITION;  // hardware clip‑space output (→ gl_Position)

				// ── user varying channels (location 0‑7, up to vec4 each) ──
				vec3 color   : TEXCOORD0;
				// vec2 uv0  : TEXCOORD1;
				// vec2 uv1  : TEXCOORD2;
				// vec3 worldNormal : TEXCOORD3;
				// vec3 worldPos    : TEXCOORD4;
				// vec3 tangent     : TEXCOORD5;
				// vec3 viewDir     : TEXCOORD6;
				// float fogFactor  : TEXCOORD7;
			};

			// ═══════════════════════════════════════
			//  Vertex shader
			// ═══════════════════════════════════════

			void vert_main(
				// ── vertex attributes consumed via a2v struct ──
				in a2v v,
				out v2f o)
			{
				// ── GPU‑driven example (uses built‑ins) ──
				// uint idx = gl_VertexIndex + gl_BaseVertex;
				// uint instance = gl_InstanceIndex + gl_BaseInstance;

				vec4 worldPos = u_Object.LocalToWorld * vec4(v.position, 1.0);
				o.clipPos = u_Frame.ViewProj * worldPos;
				o.color = v.color;
				// o.uv0   = v.uv0;
				// o.worldNormal = mat3(u_Object.LocalToWorldInverseTranspose) * v.normal;
			}

			// ═══════════════════════════════════════
			//  Fragment shader
			// ═══════════════════════════════════════

			// ── GLSL built‑in input globals (no semantic needed) ──
			//
			//   gl_FragCoord     vec4  pixel (x,y,z=NDC depth,w=1/w)
			//   gl_FrontFacing   bool  true = front face, false = back
			//   gl_PointCoord    vec2  point‑sprite UV (0‑1)
			//   gl_SampleID       int  MSAA sample index
			//   gl_SamplePosition vec2  MSAA sample position within pixel
			//   gl_SampleMaskIn   int  MSAA coverage mask input

			// ── GLSL built‑in output globals (write in shader body) ──
			//
			//   gl_FragDepth    float   override fragment depth value
			//   gl_SampleMask   int     override MSAA coverage mask

			// Fragment output semantics:
			//   : COLOR0  – render‑target 0  (always required)
			//   : COLOR1  – render‑target 1  (MRT, deferred G‑buffer)

			void frag_main(
				in v2f i,
				out vec4 outColor : COLOR0
				// out vec4 outGBuffer1 : COLOR1   // extra MRT target
				)
			{
				// ── built‑in examples ──
				// vec3 viewPos = gl_FragCoord.xyz;
				// vec3 normal = gl_FrontFacing ? worldNormal : -worldNormal;
				// gl_FragDepth = 0.5;  // override depth

				outColor = vec4(i.color * u_Material._Color.rgb, 1.0);
				// outGBuffer1 = vec4(0.0);
			}
		}
	}
}
