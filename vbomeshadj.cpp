#include "vbomeshadj.h"

#define uint unsigned int

#include <cstdlib>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <fstream>
using std::ifstream;
#include <sstream>
using std::istringstream;

#include <map>
using std::map;

VBOMeshAdj::VBOMeshAdj(const char * fileName, bool center) : nFaces(0), v(0), nVerts(0), n(0), tc(0), el(0), tang(0)
{    
    loadOBJ(fileName, center);
}

/*
void VBOMeshAdj::render() const {
    glBindVertexArray(vaoHandle);
    glDrawElements(GL_TRIANGLES_ADJACENCY, 6 * faces, GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));
}
*/

VBOMeshAdj::~VBOMeshAdj()
{
    delete [] v;
    delete [] n;
    if( tc != NULL ) delete [] tc;
    if( tang != NULL ) delete [] tang;
    delete [] el;
}

void VBOMeshAdj::determineAdjacency(vector<unsigned int> &el)
{
    // Elements with adjacency info
    vector<unsigned int> elAdj;

    // Copy and make room for adjacency info
    for( uint i = 0; i < el.size(); i+=3)
    {
        elAdj.push_back(el[i]);
        elAdj.push_back(-1);
        elAdj.push_back(el[i+1]);
        elAdj.push_back(-1);
        elAdj.push_back(el[i+2]);
        elAdj.push_back(-1);
    }

    // Find matching edges
    for( uint i = 0; i < elAdj.size(); i+=6)
    {
        // A triangle
        int a1 = elAdj[i];
        int b1 = elAdj[i+2];
        int c1 = elAdj[i+4];

        // Scan subsequent triangles
        for(uint j = i+6; j < elAdj.size(); j+=6)
        {
            int a2 = elAdj[j];
            int b2 = elAdj[j+2];
            int c2 = elAdj[j+4];

            // Edge 1 == Edge 1
            if( (a1 == a2 && b1 == b2) || (a1 == b2 && b1 == a2) )
            {
                elAdj[i+1] = c2;
                elAdj[j+1] = c1;
            }
            // Edge 1 == Edge 2
            if( (a1 == b2 && b1 == c2) || (a1 == c2 && b1 == b2) )
            {
                elAdj[i+1] = a2;
                elAdj[j+3] = c1;
            }
            // Edge 1 == Edge 3
            if ( (a1 == c2 && b1 == a2) || (a1 == a2 && b1 == c2) )
            {
                elAdj[i+1] = b2;
                elAdj[j+5] = c1;
            }
            // Edge 2 == Edge 1
            if( (b1 == a2 && c1 == b2) || (b1 == b2 && c1 == a2) )
            {
                elAdj[i+3] = c2;
                elAdj[j+1] = a1;
            }
            // Edge 2 == Edge 2
            if( (b1 == b2 && c1 == c2) || (b1 == c2 && c1 == b2) )
            {
                elAdj[i+3] = a2;
                elAdj[j+3] = a1;
            }
            // Edge 2 == Edge 3
            if( (b1 == c2 && c1 == a2) || (b1 == a2 && c1 == c2) )
            {
                elAdj[i+3] = b2;
                elAdj[j+5] = a1;
            }
            // Edge 3 == Edge 1
            if( (c1 == a2 && a1 == b2) || (c1 == b2 && a1 == a2) )
            {
                elAdj[i+5] = c2;
                elAdj[j+1] = b1;
            }
            // Edge 3 == Edge 2
            if( (c1 == b2 && a1 == c2) || (c1 == c2 && a1 == b2) )
            {
                elAdj[i+5] = a2;
                elAdj[j+3] = b1;
            }
            // Edge 3 == Edge 3
            if( (c1 == c2 && a1 == a2) || (c1 == a2 && a1 == c2) )
            {
                elAdj[i+5] = b2;
                elAdj[j+5] = b1;
            }
        }
    }

    // Look for any outside edges
    for( uint i = 0; i < elAdj.size(); i+=6)
    {
        if( elAdj[i+1] == -1 ) elAdj[i+1] = elAdj[i+4];
        if( elAdj[i+3] == -1 ) elAdj[i+3] = elAdj[i];
        if( elAdj[i+5] == -1 ) elAdj[i+5] = elAdj[i+2];
    }

    // Copy all data back into el
    el = elAdj;
}

void VBOMeshAdj::loadOBJ( const char * fileName, bool reCenterMesh ) {

  vector <QVector3D> p;
  vector <QVector3D> n;
  vector <QVector2D> tc;         // Holds tex coords from OBJ file
  vector <unsigned int> faces, faceTC;

  //int nFaces = 0;

  ifstream objStream( fileName, std::ios::in );

  if( !objStream ) {
    cerr << "Unable to open OBJ file: " << fileName << endl;
    exit(1);
  }

  cout << "Loading OBJ mesh: " << fileName << endl;
  string line, token;

  getline( objStream, line );
  while( !objStream.eof() ) {
    trimString(line);
    if( line.length( ) > 0 && line.at(0) != '#' ) {
      istringstream lineStream( line );

      lineStream >> token;

      if (token == "v" ) {
        float x, y, z;
        lineStream >> x >> y >> z;
        p.push_back( QVector3D(x,y,z) );
      } else if (token == "vt" ) {
        // Process texture coordinate
        float s,t;
        lineStream >> s >> t;
        tc.push_back( QVector2D(s,t) );
      } else if (token == "vn" ) {
        float x, y, z;
        lineStream >> x >> y >> z;
        n.push_back( QVector3D(x,y,z) );
      } else if (token == "f" ) {
        nFaces++;

        // Process face
        size_t slash1, slash2;
        int faceVerts = 0;
        while( lineStream.good() ) {
          faceVerts++;
          string vertString;
          lineStream >> vertString;
          int pIndex = -1, nIndex = -1 , tcIndex = -1;

          slash1 = vertString.find("/");
          if( slash1 == string::npos ){
            pIndex = atoi( vertString.c_str() ) - 1 ;
          } else {
            slash2 = vertString.find("/", slash1 + 1 );
            pIndex = atoi( vertString.substr(0,slash1).c_str() ) - 1;
            if( slash2 == string::npos || slash2 > slash1 + 1) {
              tcIndex =
                atoi( vertString.substr(slash1 + 1, slash2).c_str() ) - 1;
            }
            if( slash2 != string::npos )
              nIndex =
                atoi( vertString.substr(slash2 + 1,string::npos).c_str() ) - 1;
          }
          if( pIndex == -1 ) {
            printf("Missing point index!!!");
          } else {
            faces.push_back(pIndex);
          }
          if( tcIndex != -1 ) faceTC.push_back(tcIndex);

          if ( nIndex != -1 && nIndex != pIndex ) {
            printf("Normal and point indices are not consistent.\n");
          }
        }
        if( faceVerts != 3 ) {
          printf("Found non-triangular face.\n");
        }
      }
    }
    getline( objStream, line );
  }

  objStream.close();

  // 2nd pass, re-do the lists to make the indices consistent
  vector<QVector2D> texCoords;
  for( unsigned int i = 0; i < p.size(); i++ ) texCoords.push_back(QVector2D(0.0f, 0.0f));
  std::map<int, int> pToTex;
  for( unsigned int i = 0; i < faces.size(); i++ ) {
    int point = faces[i];
    int texCoord = faceTC[i];
    std::map<int, int>::iterator it = pToTex.find(point);
    if( it == pToTex.end() ) {
      pToTex[point] = texCoord;
      texCoords[point] = tc[texCoord];
    } else {
      if( texCoord != it->second ) {
        p.push_back( p[point] );  // Dup the point
        texCoords.push_back( tc[texCoord] );
        faces[i] = (unsigned int)(p.size() - 1);
      }
    }
  }

  if( n.size() == 0 ) {
    cout << "Generating normal vectors" << endl;
    generateAveragedNormals(p,n,faces);
  }

  vector<QVector4D> tangents;
  if( texCoords.size() > 0 ) {
    cout << "Generating tangents" << endl;
    generateTangents(p,n,faces,texCoords,tangents);
  }

  if( reCenterMesh ) {
    center(p);
  }

  // Determine the adjacency information
  cout << "Determining mesh adjacencies" << endl;
  determineAdjacency(faces);

  storeVBO(p, n, texCoords, tangents, faces);

  cout << "Loaded mesh from: " << fileName << endl;
  cout << " " << p.size() << " points" << endl;
  cout << " " << nFaces << " faces" << endl;
  cout << " " << n.size() << " normals" << endl;
  cout << " " << tangents.size() << " tangents " << endl;
  cout << " " << texCoords.size() << " texture coordinates." << endl;
}

void VBOMeshAdj::center( vector<QVector3D> & points ) {
    if( points.size() < 1) return;

    QVector3D maxPoint = points[0];
    QVector3D minPoint = points[0];

    // Find the AABB
    for( uint i = 0; i < points.size(); ++i ) {
        QVector3D & point = points[i];
        if( point.x() > maxPoint.x() ) maxPoint.setX(point.x());
        if( point.y() > maxPoint.y() ) maxPoint.setY(point.y());
        if( point.z() > maxPoint.z() ) maxPoint.setZ(point.z());
        if( point.x() < minPoint.x() ) minPoint.setX(point.x());
        if( point.y() < minPoint.y() ) minPoint.setY(point.y());
        if( point.z() < minPoint.z() ) minPoint.setZ(point.z());
    }

    // Center of the AABB
    QVector3D center = QVector3D( (maxPoint.x() + minPoint.x()) / 2.0f,
                        (maxPoint.y() + minPoint.y()) / 2.0f,
                        (maxPoint.z() + minPoint.z()) / 2.0f );

    // Translate center of the AABB to the origin
    for( uint i = 0; i < points.size(); ++i ) {
        QVector3D & point = points[i];
        point = point - center;
    }
}

void VBOMeshAdj::generateAveragedNormals(
        const vector<QVector3D> & points,
        vector<QVector3D> & normals,
        const vector<unsigned int> & faces )
{
    for( uint i = 0; i < points.size(); i++ ) {
        normals.push_back(QVector3D());
    }

    for( uint i = 0; i < faces.size(); i += 3) {
        const QVector3D & p1 = points[faces[i]];
        const QVector3D & p2 = points[faces[i+1]];
        const QVector3D & p3 = points[faces[i+2]];

        QVector3D a = p2 - p1;
        QVector3D b = p3 - p1;
        QVector3D n = QVector3D::crossProduct(a,b).normalized();

        normals[faces[i]] += n;
        normals[faces[i+1]] += n;
        normals[faces[i+2]] += n;
    }

    for( uint i = 0; i < normals.size(); i++ ) {
        normals[i].normalize();
    }
}

void VBOMeshAdj::generateTangents(
        const vector<QVector3D> & points,
        const vector<QVector3D> & normals,
        const vector<unsigned int> & faces,
        const vector<QVector2D> & texCoords,
        vector<QVector4D> & tangents)
{
    vector<QVector3D> tan1Accum;
    vector<QVector3D> tan2Accum;

    for( uint i = 0; i < points.size(); i++ ) {
        tan1Accum.push_back(QVector3D());
        tan2Accum.push_back(QVector3D());
        tangents.push_back(QVector4D());
    }

    // Compute the tangent vector
    for( uint i = 0; i < faces.size(); i += 3 )
    {
        const QVector3D &p1 = points[faces[i]];
        const QVector3D &p2 = points[faces[i+1]];
        const QVector3D &p3 = points[faces[i+2]];

        const QVector2D &tc1 = texCoords[faces[i]];
        const QVector2D &tc2 = texCoords[faces[i+1]];
        const QVector2D &tc3 = texCoords[faces[i+2]];

        QVector3D q1 = p2 - p1;
        QVector3D q2 = p3 - p1;
        float s1 = tc2.x() - tc1.x(), s2 = tc3.x() - tc1.x();
        float t1 = tc2.y() - tc1.y(), t2 = tc3.y() - tc1.y();
        float r = 1.0f / (s1 * t2 - s2 * t1);
        QVector3D tan1( (t2*q1.x() - t1*q2.x()) * r,
                   (t2*q1.y() - t1*q2.y()) * r,
                   (t2*q1.z() - t1*q2.z()) * r);
        QVector3D tan2( (s1*q2.x() - s2*q1.x()) * r,
                   (s1*q2.y() - s2*q1.y()) * r,
                   (s1*q2.z() - s2*q1.z()) * r);
        tan1Accum[faces[i]] += tan1;
        tan1Accum[faces[i+1]] += tan1;
        tan1Accum[faces[i+2]] += tan1;
        tan2Accum[faces[i]] += tan2;
        tan2Accum[faces[i+1]] += tan2;
        tan2Accum[faces[i+2]] += tan2;
    }

    for( uint i = 0; i < points.size(); ++i )
    {
        const QVector3D &n = normals[i];
        QVector3D &t1 = tan1Accum[i];
        QVector3D &t2 = tan2Accum[i];

        // Gram-Schmidt orthogonalize
        tangents[i] = QVector4D(( t1 - (QVector3D::dotProduct(n,t1) * n) ).normalized(), 0.0f).normalized();
        // Store handedness in w
        tangents[i].setW( (QVector3D::dotProduct(QVector3D::crossProduct(n,t1), t2 ) < 0.0f) ? -1.0f : 1.0f );
    }
    tan1Accum.clear();
    tan2Accum.clear();
}

void VBOMeshAdj::storeVBO( const vector<QVector3D> & points,
                        const vector<QVector3D> & normals,
                        const vector<QVector2D> &texCoords,
                        const vector<QVector4D> &tangents,
                        const vector<unsigned int> &elements )
{
    nVerts = (unsigned int) points.size();
    nFaces = (unsigned int) elements.size() / 6;

    v = new float[3 * nVerts];
    n = new float[3 * nVerts];

    if(texCoords.size() > 0 && tangents.size() > 0) {
        tc = new float[ 2 * nVerts];
        tang = new float[4*nVerts];
    }

    el = new unsigned int[elements.size()];
    int idx = 0, tcIdx = 0, tangIdx = 0;
    for( unsigned int i = 0; i < nVerts; ++i )
    {
        v[idx] = points[i].x();
        v[idx+1] = points[i].y();
        v[idx+2] = points[i].z();
        n[idx] = normals[i].x();
        n[idx+1] = normals[i].y();
        n[idx+2] = normals[i].z();
        idx += 3;
        if( tc != NULL ) {
            tang[tangIdx] = tangents[i].x();
            tang[tangIdx+1] = tangents[i].y();
            tang[tangIdx+2] = tangents[i].z();
            tang[tangIdx+3] = tangents[i].w();
            tangIdx += 4;
            tc[tcIdx] = texCoords[i].x();
            tc[tcIdx+1] = texCoords[i].y();
            tcIdx += 2;
        }
    }
    for( unsigned int i = 0; i < elements.size(); ++i )
    {
        el[i] = elements[i];
    }
    printf("End storeVBO\n");
}

void VBOMeshAdj::trimString( string & str ) {
    const char * whiteSpace = " \t\n\r";
    size_t location;
    location = str.find_first_not_of(whiteSpace);
    str.erase(0,location);
    location = str.find_last_not_of(whiteSpace);
    str.erase(location + 1);
}

float *VBOMeshAdj::getv()
{
    return v;
}

unsigned int VBOMeshAdj::getnVerts()
{
    return nVerts;
}

float *VBOMeshAdj::getn()
{
    return n;
}

float *VBOMeshAdj::gettc()
{
    return tc;
}

unsigned int *VBOMeshAdj::getelems()
{
    return el;
}

unsigned int VBOMeshAdj::getnFaces()
{
    return nFaces;
}

float *VBOMeshAdj::gettang()
{
    return tang;
}

