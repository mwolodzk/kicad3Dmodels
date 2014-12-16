/*
 *      file: kc3dtess.h
 *
 *      Copyright 2012-2014 Dr. Cirilo Bernardo (cjh.bernardo@gmail.com)
 *
 *      This program is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *
 *      This class creates a tessellated surface given a solid outline in the
 *      XY plane and a list of cutouts.
 *
 */

#ifndef KC3DTESS_H
#define KC3DTESS_H

#include <wx/glcanvas.h>    // CALLBACK definition, needed on Windows
// alse needed on OSX to define __DARWIN__

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif

#include <fstream>
#include <vector>
#include <list>
#include <utility>

#ifndef M_PI2
#define M_PI2 ( M_PI / 2.0 )
#endif

#ifndef M_PI4
#define M_PI4 ( M_PI / 4.0 )
#endif

namespace KC3D
{
    class VRMLMAT;
    class TRANSFORM;
    class QUAT;
    class POLYGON;


    struct VERTEX_3D
    {
        double  x;
        double  y;
        int i;          // vertex index
        int o;          // vertex order
    };


    struct TRIPLET_3D
    {
        int i1, i2, i3;

        TRIPLET_3D( int p1, int p2, int p3 )
        {
            i1  = p1;
            i2  = p2;
            i3  = p3;
        }
    };


    class TESSELATOR
    {
    private:
        // Arc parameters
        int    maxArcSeg;                       // maximum number of arc segments in a small circle
        double minSegLength;                    // min. segment length
        double maxSegLength;                    // max. segment length

        bool    fix;                            // when true, no more vertices may be added by the user
        int     idx;                            // vertex index (number of contained vertices)
        int     ord;                            // vertex order (number of ordered vertices)
        unsigned int idxout;                    // outline index to first point in 3D outline
        std::vector<VERTEX_3D*> vertices;       // vertices of all contours
        std::vector<std::list<int>*> contours;  // lists of vertices for each contour
        std::vector< double > areas;            // area of the contours (positive if winding is CCW)
        std::list<TRIPLET_3D> triplets;         // output facet triplet list (triplet of ORDER values)
        std::list<std::list<int>*> outline;     // indices for outline outputs (index by ORDER values)
        std::vector<int> ordmap;                // mapping of ORDER to INDEX

        std::string error;                      // error message

        int eidx;                               // index for extra vertices
        std::vector<VERTEX_3D*> extra_verts;    // extra vertices added for outlines and facets
        std::vector<VERTEX_3D*> vlist;          // vertex list for the GL command in progress
        TESSELATOR* pholes;                     // pointer to another layer object used for tesselation;
                                                // this object is normally expected to hold only holes

        GLUtesselator* tess;                    // local instance of the GLU tesselator

        GLenum glcmd;                           // current GL command type ( fan, triangle, tri-strip, loop )

        void clearTmp( void );                  // clear ephemeral data used by the tesselation routine

        // add a triangular facet (triplet) to the output index list
        bool addTriplet( VERTEX_3D* p0, VERTEX_3D* p1, VERTEX_3D* p2 );

        // retrieve a vertex given its index; the vertex may be contained in the
        // vertices vector, extra_verts vector, or foreign TESSELATOR object
        VERTEX_3D* getVertexByIndex( int aPointIndex );

        void    processFan( void );                 // process a GL_TRIANGLE_FAN list
        void    processStrip( void );               // process a GL_TRIANGLE_STRIP list
        void    processTri( void );                 // process a GL_TRIANGLES list

        void pushVertices( bool holes );            // push the internal vertices
        bool pushOutline( void );                   // push the (GL generated) outline vertices

        // returns the number of solid or hole contours
        int checkNContours( bool holes );

    public:
        /// set to true when a fault is encountered during tesselation
        bool Fault;

        TESSELATOR();
        virtual ~TESSELATOR();

        /**
         * Function Clear
         * erases all tesselation data
         */
        void Clear( void );

        /**
         * Function GetSize
         * returns the total number of vertices indexed
         */
        int GetSize( void );

        /**
         * Function GetNConours
         * returns the number of stored contours
         */
        int GetNContours( void )
        {
            return contours.size();
        }

        /**
         * Function NewContour
         * creates a new list of vertices and returns an index to the list
         *
         * @return int: index to the list or -1 if the operation failed
         */
        int NewContour( void );

        /**
         * Function AddVertex
         * adds a point to the requested contour
         *
         * @param aContour is an index previously returned by a call to NewContour()
         * @param aXpos is the X coordinate of the vertex
         * @param aYpos is the Y coordinate of the vertex
         *
         * @return bool: true if the vertex was added
         */
        bool AddVertex( int aContourID, double aXpos, double aYpos );

        /**
         * Function AddPolygon
         * adds contents of a polygon to the requested contour
         *
         * @param aPolygon is a polygon assumed to be on the XY plane (Z data is ignored)
         * @param aHoleFlag determines if the resulting contour must be a hole
         *
         * @return bool: true if the vertex was added
         */
        bool AddPolygon( const KC3D::POLYGON& aPolygon, bool aHoleFlag );

        /**
         * Function EnsureWinding
         * checks the winding of a contour and ensures that it is a hole or
         * a solid depending on the value of @param hole
         *
         * @param aContour is an index to a contour as returned by NewContour()
         * @param aHoleFlag determines if the contour must be a hole
         *
         * @return bool: true if the operation suceeded
         */
        bool EnsureWinding( int aContourID, bool aHoleFlag );

    private:
        /**
         * Function Tesselate
         * creates a list of outline vertices as well as the
         * vertex sets required to render the surface.
         *
         * @return bool: true if the operation succeeded
         */
        bool Tesselate( void );

        /**
         * Function WriteVertices
         * writes out the list of vertices required to render a
         * planar surface.
         *
         * @param aOutFile is the file to write to
         * @param aPrecision is the precision of the output coordinates
         * @param aTransform is the transform to apply to output vertices
         *
         * @return bool: true if the operation succeeded
         */
        bool WriteVertices( std::ofstream& aOutFile, int aPrecision,
                            KC3D::TRANSFORM& aTransform, int aTabDepth );

        /**
         * Function WriteIndices
         * writes out the vertex sets required to render a planar
         * surface.
         *
         * @param aTopFlag is true if the surface is to be visible from above;
         * if false the surface will be visible from below.
         * @param aOutFile is the file to write to
         *
         * @return bool: true if the operation succeeded
         */
        bool WriteIndices( bool aTopFlag, std::ofstream& aOutFile, int aTabDepth );

    public:
        /**
         * Function AddExtraVertex
         * adds an extra vertex as required by the GLU tesselator.
         *
         * @param aXpos is the X coordinate of the newly created point
         * @param aYpos is the Y coordinate of the newly created point
         *
         * @return VERTEX_3D*: is the new vertex or NULL if a vertex
         * could not be created.
         */
        VERTEX_3D* AddExtraVertex( double aXpos, double aYpos );

        /**
         * Function glStart
         * is invoked by the GLU tesselator callback to notify this object
         * of the type of GL command which is applicable to the upcoming
         * vertex list.
         *
         * @param cmd is the GL command
         */
        void glStart( GLenum cmd );

        /**
         * Function glPushVertex
         * is invoked by the GLU tesselator callback; the supplied vertex is
         * added to the internal list of vertices awaiting processing upon
         * execution of glEnd()
         *
         * @param vertex is a vertex forming part of the GL command as previously
         * set by glStart
         */
        void glPushVertex( VERTEX_3D* vertex );

        /**
         * Function glEnd
         * is invoked by the GLU tesselator callback to notify this object
         * that the vertex list is complete and ready for processing
         */
        void glEnd( void );

        /**
         * Function SetGLError
         * sets the error message according to the specified OpenGL error
         */
        void SetGLError( GLenum error_id );

        /**
         * Function GetVertexByIndex
         * returns a pointer to the requested vertex or
         * NULL if no such vertex exists.
         *
         * @param aPointIndex is a vertex index
         *
         * @return VERTEX_3D*: the requested vertex or NULL
         */
        VERTEX_3D* GetVertexByIndex( int aPointIndex );

        /*
         * Function GetError
         * Returns the error message related to the last failed operation
         */
        const std::string& GetError( void );

        /**
         * \ Brief write the tesselated surface to a VRML file
         *
         * @param isCCW           [in] true to render the outer (upper) surface visible
         * @param aTransform      [in] transform to apply to output vertices
         * @param aMaterial       [in] appearance specification
         * @param reuseMaterial   [in] true to reuse @param aMaterial
         * @param aVRMLFile       [in] open output file
         * @param aTabDepth       [in] formatting indent level
         * @return true for success, false for failure
         */
        bool WriteVRML( bool isCCW, TRANSFORM& aTransform, VRMLMAT& aMaterial,
                        bool reuseMaterial, std::ofstream& aVRMLFile, int aTabDepth );
    };

}   // namespace KC3D

#endif  // KC3DTESS_H
