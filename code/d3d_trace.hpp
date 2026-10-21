// Optional GL call trace + per-draw D3D state log (debugging aid, off by default).
// Enable with QindieGL.ini [Settings] Trace=1|2|3 or env QINDIEGL_TRACE (env wins). See doc/TRACE.md.
//   1 = one line per D3D draw call (textures, matrices, FVF, ortho/3D, render states) + per-frame summary
//   2 = also the GL calls that build that state (bind, matrix ops, viewport, begin/end, draw*)
//   3 = like 2, with the full world/view/projection matrices on each draw
#ifndef QINDIEGL_D3D_TRACE_H
#define QINDIEGL_D3D_TRACE_H

extern int g_qtraceLevel;           // 0 = off
void qtrace_init();                 // read settings, call once the ini is loaded
void qtrace_shutdown();
void qtrace_gl( const char *fmt, ... );
void qtrace_bind( GLuint texture ); // remembers the GL name for the draw log
void qtrace_draw( const char *src, GLenum glmode, int count );
void qtrace_frame_end();

#define QTRACE_GL( ... )  do { if ( g_qtraceLevel >= 2 ) qtrace_gl( __VA_ARGS__ ); } while (0)

#endif
