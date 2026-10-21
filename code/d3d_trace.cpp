// Optional GL call trace + per-draw D3D state log. See d3d_trace.hpp.
#include "d3d_wrapper.hpp"
#include "d3d_global.hpp"
#include "d3d_state.hpp"
#include "d3d_trace.hpp"
#include <stdarg.h>
#include <stdlib.h>

int g_qtraceLevel = 0;

static FILE  *s_fp = NULL;
static DWORD  s_frame = 0;        // frames presented so far
static DWORD  s_skip = 0;         // frames to skip before logging
static DWORD  s_count = 3;        // frames to log (0 = unlimited)
static DWORD  s_draws = 0, s_ortho = 0, s_3d = 0;
static GLuint s_boundTex[MAX_D3D_TMU];
static bool   s_active = false;   // inside a logged frame

static DWORD envOrIni( const char *env, const char *key, DWORD def )
{
	const char *e = getenv( env );
	if ( e && *e ) return (DWORD)strtoul( e, NULL, 0 );
	return D3DGlobal_GetRegistryValue( key, "Settings", def );
}

static void updateActive()
{
	s_active = g_qtraceLevel > 0 && s_frame >= s_skip && ( !s_count || s_frame < s_skip + s_count );
	if ( s_active && !s_fp ) {
		s_fp = fopen( "QindieGL_trace.log", "w" );
		if ( !s_fp ) g_qtraceLevel = s_active = 0;
	}
}

void qtrace_init()
{
	g_qtraceLevel = (int)envOrIni( "QINDIEGL_TRACE", "Trace", 0 );
	s_skip = envOrIni( "QINDIEGL_TRACE_SKIP", "TraceSkipFrames", 0 );
	s_count = envOrIni( "QINDIEGL_TRACE_FRAMES", "TraceFrames", 3 );
	memset( s_boundTex, 0, sizeof( s_boundTex ) );
	if ( g_qtraceLevel )
		logPrintf( "Trace level %d, skip %u frames, log %u frames -> QindieGL_trace.log\n", g_qtraceLevel, s_skip, s_count );
	updateActive();
}

void qtrace_shutdown()
{
	if ( s_fp ) { fclose( s_fp ); s_fp = NULL; }
}

void qtrace_gl( const char *fmt, ... )
{
	if ( !s_active || !s_fp ) return;
	va_list ap;
	va_start( ap, fmt );
	fputs( "  gl ", s_fp );
	vfprintf( s_fp, fmt, ap );
	fputc( '\n', s_fp );
	va_end( ap );
}

void qtrace_bind( GLuint texture )
{
	int tmu = D3DState.TextureState.currentTMU;
	if ( tmu >= 0 && tmu < MAX_D3D_TMU ) s_boundTex[tmu] = texture;
}

static unsigned matHash( const D3DMATRIX &m )
{
	const unsigned char *p = (const unsigned char*)&m;
	unsigned h = 2166136261u;
	for ( int i = 0; i < (int)sizeof( m ); ++i ) { h ^= p[i]; h *= 16777619u; }
	return h;
}

static void printMat( const char *name, const D3DMATRIX &m )
{
	fprintf( s_fp, "    %s %08x\n      [% .4f % .4f % .4f % .4f]\n      [% .4f % .4f % .4f % .4f]\n      [% .4f % .4f % .4f % .4f]\n      [% .4f % .4f % .4f % .4f]\n",
		name, matHash( m ),
		m._11, m._12, m._13, m._14, m._21, m._22, m._23, m._24,
		m._31, m._32, m._33, m._34, m._41, m._42, m._43, m._44 );
}

static const char *primName( GLenum m )
{
	switch ( m ) {
	case GL_POINTS: return "POINTS"; case GL_LINES: return "LINES"; case GL_LINE_STRIP: return "LSTRIP";
	case GL_LINE_LOOP: return "LLOOP"; case GL_TRIANGLES: return "TRIS"; case GL_TRIANGLE_STRIP: return "TSTRIP";
	case GL_TRIANGLE_FAN: return "TFAN"; case GL_QUADS: return "QUADS"; case GL_QUAD_STRIP: return "QSTRIP";
	case GL_POLYGON: return "POLY"; default: return "?";
	}
}

void qtrace_draw( const char *src, GLenum glmode, int count )
{
	if ( !s_active || !s_fp || !D3DGlobal.pDevice ) return;
	IDirect3DDevice9 *dev = D3DGlobal.pDevice;

	D3DMATRIX w, v, p;
	dev->GetTransform( D3DTS_WORLD, &w );
	dev->GetTransform( D3DTS_VIEW, &v );
	dev->GetTransform( D3DTS_PROJECTION, &p );
	bool d3dOrtho = p._44 == 1.0f && p._34 == 0.0f;  // perspective has _34 = +-1, _44 = 0
	bool glOrtho = D3DGlobal_IsOrthoProjection();

	DWORD fvf = 0, zen = 0, zw = 0, ab = 0, sb = 0, db = 0, cull = 0, at = 0, lighting = 0;
	dev->GetFVF( &fvf );
	dev->GetRenderState( D3DRS_ZENABLE, &zen );
	dev->GetRenderState( D3DRS_ZWRITEENABLE, &zw );
	dev->GetRenderState( D3DRS_ALPHABLENDENABLE, &ab );
	dev->GetRenderState( D3DRS_SRCBLEND, &sb );
	dev->GetRenderState( D3DRS_DESTBLEND, &db );
	dev->GetRenderState( D3DRS_CULLMODE, &cull );
	dev->GetRenderState( D3DRS_ALPHATESTENABLE, &at );
	dev->GetRenderState( D3DRS_LIGHTING, &lighting );
	IDirect3DVertexShader9 *vs = NULL; dev->GetVertexShader( &vs ); if ( vs ) vs->Release();
	IDirect3DPixelShader9 *ps = NULL; dev->GetPixelShader( &ps ); if ( ps ) ps->Release();
	D3DVIEWPORT9 vp; dev->GetViewport( &vp );

	++s_draws;
	if ( d3dOrtho ) ++s_ortho; else ++s_3d;
	fprintf( s_fp, "D%u %s %s n=%d %s%s fvf=%08x z=%u/%u blend=%u(%u,%u) atest=%u cull=%u light=%u vs=%d ps=%d vp=%ux%u W=%08x V=%08x P=%08x",
		s_draws, src, primName( glmode ), count, d3dOrtho ? "ORTHO" : "3D", d3dOrtho != glOrtho ? "(!gl)" : "",
		fvf, zen, zw, ab, sb, db, at, cull, lighting, vs != NULL, ps != NULL, vp.Width, vp.Height,
		matHash( w ), matHash( v ), matHash( p ) );
	for ( int i = 0; i < MAX_D3D_TMU; ++i ) {
		IDirect3DBaseTexture9 *t = NULL;
		dev->GetTexture( i, &t );
		if ( !t ) continue;
		fprintf( s_fp, " tex%d=gl%u/%p", i, s_boundTex[i], (void*)t );
		t->Release();
	}
	fputc( '\n', s_fp );
	if ( g_qtraceLevel >= 3 ) {
		printMat( "W", w ); printMat( "V", v ); printMat( "P", p );
	}
}

void qtrace_frame_end()
{
	if ( s_active && s_fp ) {
		fprintf( s_fp, "=== frame %u: %u draws (%u 3D, %u ortho) ===\n", s_frame, s_draws, s_3d, s_ortho );
		fflush( s_fp );
	}
	s_draws = s_ortho = s_3d = 0;
	++s_frame;
	updateActive();
	if ( s_active && s_fp ) fprintf( s_fp, "=== begin frame %u ===\n", s_frame );
}
