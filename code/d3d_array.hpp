/***************************************************************************
* Copyright (C) 2011-2016, Crystice Softworks.
* 
* This file is part of QindieGL source code.
* Please note that QindieGL is not driver, it's emulator.
* 
* QindieGL source code is free software; you can redistribute it and/or 
* modify it under the terms of the GNU General Public License as 
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* QindieGL source code is distributed in the hope that it will be 
* useful, but WITHOUT ANY WARRANTY; without even the implied 
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
* See the GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software 
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
***************************************************************************/
#ifndef	QINDIEGL_D3D_ARRAY_H
#define QINDIEGL_D3D_ARRAY_H

class D3DVABuffer
{
	static const GLsizei c_MaxSwapFrame = 8;
public:
	D3DVABuffer();
	~D3DVABuffer();
	void Lock( GLint first, GLint last );
	void Unlock();
	template<typename T> void SetIndices( GLenum mode, GLuint start, GLuint end, GLsizei count, const T *indices );
	void DrawPrimitive();
	void ResetSwapFrame() { m_swapFrame = 0; }

	GLint GetLockFirst() const { return m_lockFirst; }
	GLsizei GetLockCount() const { return m_lockCount; }

protected:
	void SetMinimumVertexBufferSize( GLsizei numVerts );
	int  SetMinimumIndexBufferSize( GLsizei numIndices );
	void SetupTexCoords( const float *texcoords, int num_coords, const float *position, const float *normal, int stage, float *out_texcoords );

	inline void SetIndex( void *pDest, GLuint dstIndex, GLsizei srcIndex )
	{
		if (m_indexSize == 4)
			*((GLuint*)pDest + dstIndex) = (GLuint)srcIndex;
		else
			*((GLushort*)pDest + dstIndex) = (GLushort)srcIndex;
	}
	template<typename T>
	inline void SetIndex( void *pDest, GLuint dstIndex, T srcIndex )
	{
		if (m_indexSize == 4)
			*((GLuint*)pDest + dstIndex) = (GLuint)srcIndex;
		else
			*((GLushort*)pDest + dstIndex) = (GLushort)srcIndex;
	}

private:
	LPDIRECT3DVERTEXBUFFER9		m_pVertexBuffer[c_MaxSwapFrame];
	LPDIRECT3DINDEXBUFFER9		m_pIndexBuffer[2][c_MaxSwapFrame];
	GLsizei						m_vbAllocSize[c_MaxSwapFrame];
	GLsizei						m_ibAllocSize[2][c_MaxSwapFrame];
	GLsizei						m_vertexSize;
	GLsizei						m_indexSize;
	GLint						m_lockFirst;
	GLsizei						m_lockCount;
	GLenum						m_primitiveType;
	GLsizei						m_primitiveIndexCount;
	GLint						m_swapFrame;
	// Sub-allocation (D3DLOCK_NOOVERWRITE): every draw appends to its buffer instead of discarding it; DISCARD only when the buffer is full (see Lock / SetIndices).
	GLsizei						m_vbHead[c_MaxSwapFrame];		// next free float of each vertex buffer
	GLsizei						m_ibHead[2][c_MaxSwapFrame];	// next free byte of each index buffer
	GLsizei						m_vbStartBytes;				// where this draw's vertices start in the vertex buffer (stream offset)
	GLsizei						m_ibStart;						// where this draw's indices start in the index buffer (in indices)
};

#endif //QINDIEGL_D3D_ARRAY_H