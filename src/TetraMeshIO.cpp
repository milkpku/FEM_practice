#include <iostream>

#include "Types.h"
#include "TetraMesh.h"
#include "TetraMeshIO.h"

using namespace std;

namespace BallonFEM{

int TetraMeshIO::read( istream& in, TetraMesh& tetra )
{
    TetraMeshData *data;
	data = new TetraMeshData();
    cout << "Reading from .obj file.\n";
    if ( readTetraData( in, *data ))
    {
        return 1;
    }

    cout << "Building tetra mesh.\n";
    if ( buildTetra( *data, tetra ))
    {
        return 1;
    }

    return 0;
}

int TetraMeshIO::readTetraData(istream& in, TetraMeshData& data)
{
    string line;
   
	data.vertices.clear();
	data.tetrahedrons.clear();

    while( getline( in, line ))
    {
        stringstream ss( line );
        string token;
   
        ss >> token;
   
        if( token == "v"  ) { readPosition( ss, data ); continue; } // vertex
        if( token == "t"  ) { readTetra( ss, data );    continue; } // face
		if( token == "x"  ) { readFixed( ss, data );    continue; } // fixed vertices
        if( token == "r"  ) { readRigid( ss, data );    continue; } // rigid body
        if( token[0] == '#' ) continue; // comment
        if( token == "o" ) continue; // object name
        if( token == "g" ) continue; // group name
        if( token == "s" ) continue; // smoothing group
        if( token == "mtllib" ) continue; // material library
        if( token == "usemtl" ) continue; // material
        if( token == "" ) continue; // empty string

        cerr << "Error: does not appear to be a valid Wavefront OBJ file!" << endl;
        cerr << "(Offending line: " << line << ")" << endl;
        return 1;
    }

    return 0;
}

void TetraMeshIO::readPosition( stringstream& ss, TetraMeshData& data)
{
    double x, y, z;
    ss >> x >> y >> z;
    data.vertices.push_back( Vec3(x, y, z) );
}

void TetraMeshIO::readTetra( stringstream& ss, TetraMeshData& data)
{
    int x, y, z, w;
    ss >> x >> y >> z >> w;
	data.tetrahedrons.push_back(iVec4(x, y, z, w));
}

void TetraMeshIO::readFixed( stringstream& ss, TetraMeshData& data)
{
    size_t f_id;
    while (ss >> f_id)
    {
        data.fixed.push_back(f_id);    
    }
}

void TetraMeshIO::readRigid( stringstream& ss, TetraMeshData& data)
{
    std::vector<size_t> rig;
    size_t f_id;
    while (ss >> f_id)
    {
        rig.push_back(f_id);    
    }

    data.rigid.push_back(rig);
}

int TetraMeshIO::buildTetra( TetraMeshData& data, TetraMesh& tetra)
{
    int indexBias = 1;
    /* pre-allocate */
    tetra.vertices.clear();
    tetra.tetrahedrons.clear();
    tetra.surface.clear();
    tetra.vertices.reserve( data.vertices.size() );
    tetra.tetrahedrons.reserve( data.tetrahedrons.size() );

    for(size_t i = 0; i < data.vertices.size(); i++)
    {
        Vertex nv( data.vertices[i] );
        nv.id = i;
        tetra.vertices.push_back( nv );
    }

    int v_size = tetra.vertices.size();
    for(size_t i = 0; i < data.tetrahedrons.size(); i++)
    {
        Tetra t;
        iVec4 &indeces = data.tetrahedrons[i];
        for(size_t j = 0; j < 4; j++)
        {
            /* stack overflow */
			if (indeces[j] - indexBias > v_size)
			{
				printf(" tetra index out of vertices range.\n");
				return 1;
			}
            
            t.v[j] = &tetra.vertices[ indeces[j] - indexBias ];
        }

        tetra.tetrahedrons.push_back( t );
    }

    return 0;
}

void TetraMeshIO::write( ostream& out, const TetraMesh& tetra)
{
    int indexBias = 1;
    for(VCIter v = tetra.vertices.begin(); v != tetra.vertices.end(); v++)
    {
		Vec3 pos = v->m_pos;
		out << "v " << pos[0] << " " << pos[1] << " " << pos[2] << endl; 
    }

    for(TCIter t = tetra.tetrahedrons.begin(); t !=tetra.tetrahedrons.end(); t++)
    {
        out << "t ";
        out << t->v[0]->id + indexBias << " ";
        out << t->v[1]->id + indexBias << " ";
        out << t->v[2]->id + indexBias << " ";
        out << t->v[3]->id + indexBias << endl;
    }

    out << "x";
    for(size_t i = 0; i < tetra.fixed_ids.size(); i++)
        out << " " << tetra.fixed_ids[i] + indexBias;
    out << endl;
    
    for(RCIter r = tetra.rigid_bodies.begin(); r != tetra.rigid_bodies.end(); r++)
    {
        out << "r";
        for(size_t i = 0; i < r->elements.size(); i++)
            out << r->elements[i] + indexBias;
        out << endl;
    }
    
     for(HCIter h = tetra.holes.begin(); h != tetra.holes.end(); h++)
    {
        out << "h";
        for(size_t i = 0; i < h->vertices.size(); i++)
            out << h->vertices[i] + indexBias;
        out << endl;
    }
}

}
