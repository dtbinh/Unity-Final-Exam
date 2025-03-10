
#pragma warning(disable:4996)

#include <fstream>
#include <string.h>

#include "BVH.h"



BVH::BVH()
{
	motion = NULL;
	Clear();
}


BVH::BVH( const char * bvh_file_name )
{
	motion = NULL;
	Clear();

	Load( bvh_file_name );
}


BVH::~BVH()
{
	Clear();
}



void  BVH::Clear()
{
	unsigned int  i;
	for ( i=0; i<channels.size(); i++ )
		delete  channels[ i ];
	for ( i=0; i<joints.size(); i++ )
		delete  joints[ i ];
	if ( motion != NULL )
		delete  motion;

	is_load_success = false;
	
	file_name = "";
	motion_name = "";
	
	num_channel = 0;
	channels.clear();
	joints.clear();
	joint_index.clear();
	
	num_frame = 0;
	interval = 0.0;
	motion = NULL;
}




void  BVH::Load( const char * bvh_file_name )
{
	#define  BUFFER_LENGTH  1024*4

	ifstream  file;
	char      line[ BUFFER_LENGTH ];
	char *    token;
	char      separater[] = " :,\t";
	vector< Joint * >   joint_stack;
	Joint *   joint = NULL;
	Joint *   new_joint = NULL;
	bool      is_site = false;
	double    x, y ,z;
	int       i, j;


	Clear();


	file_name = bvh_file_name;
	const char *  mn_first = bvh_file_name;
	const char *  mn_last = bvh_file_name + strlen( bvh_file_name );
	if ( strrchr( bvh_file_name, '\\' ) != NULL )
		mn_first = strrchr( bvh_file_name, '\\' ) + 1;
	else if ( strrchr( bvh_file_name, '/' ) != NULL )
		mn_first = strrchr( bvh_file_name, '/' ) + 1;
	if ( strrchr( bvh_file_name, '.' ) != NULL )
		mn_last = strrchr( bvh_file_name, '.' );
	if ( mn_last < mn_first )
		mn_last = bvh_file_name + strlen( bvh_file_name );


	motion_name.assign( mn_first, mn_last );


	file.open( bvh_file_name, ios::in );
	if ( file.is_open() == 0 )  return;

	
	while ( ! file.eof() )
	{

		if ( file.eof() )  goto bvh_error;


		file.getline( line, BUFFER_LENGTH );
		token = strtok( line, separater );


		if ( token == NULL )  continue;


		if ( strcmp( token, "{" ) == 0 )
		{

			joint_stack.push_back( joint );
			joint = new_joint;
			continue;
		}

		if ( strcmp( token, "}" ) == 0 )
		{

			joint = joint_stack.back();
			joint_stack.pop_back();
			is_site = false;
			continue;
		}


		if ( ( strcmp( token, "ROOT" ) == 0 ) ||
		     ( strcmp( token, "JOINT" ) == 0 ) )
		{

			new_joint = new Joint();
			new_joint->index = joints.size();
			new_joint->parent = joint;
			new_joint->has_site = false;
			new_joint->offset[0] = 0.0;  new_joint->offset[1] = 0.0;  new_joint->offset[2] = 0.0;
			new_joint->site[0] = 0.0;  new_joint->site[1] = 0.0;  new_joint->site[2] = 0.0;
			joints.push_back( new_joint );
			if ( joint )
				joint->children.push_back( new_joint );


			token = strtok( NULL, "" );
			while ( *token == ' ' )  token ++;
			new_joint->name = token;


			joint_index[ new_joint->name ] = new_joint;
			continue;
		}


		if ( ( strcmp( token, "End" ) == 0 ) )
		{
			new_joint = joint;
			is_site = true;
			continue;
		}


		if ( strcmp( token, "OFFSET" ) == 0 )
		{

			token = strtok( NULL, separater );
			x = token ? atof( token ) : 0.0;
			token = strtok( NULL, separater );
			y = token ? atof( token ) : 0.0;
			token = strtok( NULL, separater );
			z = token ? atof( token ) : 0.0;
			

			if ( is_site )
			{
				joint->has_site = true;
				joint->site[0] = x;
				joint->site[1] = y;
				joint->site[2] = z;
			}
			else

			{
				joint->offset[0] = x;
				joint->offset[1] = y;
				joint->offset[2] = z;
			}
			continue;
		}


		if ( strcmp( token, "CHANNELS" ) == 0 )
		{

			token = strtok( NULL, separater );
			joint->channels.resize( token ? atoi( token ) : 0 );


			for ( i=0; i< (int) joint->channels.size(); i++ )
			{
				
				Channel *  channel = new Channel();
				channel->joint = joint;
				channel->index = channels.size();
				channels.push_back( channel );
				joint->channels[ i ] = channel;

				
				token = strtok( NULL, separater );
				if ( strcmp( token, "Xrotation" ) == 0 )
					channel->type = X_ROTATION;
				else if ( strcmp( token, "Yrotation" ) == 0 )
					channel->type = Y_ROTATION;
				else if ( strcmp( token, "Zrotation" ) == 0 )
					channel->type = Z_ROTATION;
				else if ( strcmp( token, "Xposition" ) == 0 )
					channel->type = X_POSITION;
				else if ( strcmp( token, "Yposition" ) == 0 )
					channel->type = Y_POSITION;
				else if ( strcmp( token, "Zposition" ) == 0 )
					channel->type = Z_POSITION;
			}
		}

		
		if ( strcmp( token, "MOTION" ) == 0 )
			break;
	}


	
	file.getline( line, BUFFER_LENGTH );
	token = strtok( line, separater );
	if ( strcmp( token, "Frames" ) != 0 )  goto bvh_error;
	token = strtok( NULL, separater );
	if ( token == NULL )  goto bvh_error;
	num_frame = atoi( token );

	file.getline( line, BUFFER_LENGTH );
	token = strtok( line, ":" );
	if ( strcmp( token, "Frame Time" ) != 0 )  goto bvh_error;
	token = strtok( NULL, separater );
	if ( token == NULL )  goto bvh_error;
	interval = atof( token );

	num_channel = channels.size();
	motion = new double[ num_frame * num_channel ];

	
	for ( i=0; i<num_frame; i++ )
	{
		file.getline( line, BUFFER_LENGTH );
		token = strtok( line, separater );
		for ( j=0; j<num_channel; j++ )
		{
			if ( token == NULL )
				goto bvh_error;
			motion[ i*num_channel + j ] = atof( token );
			token = strtok( NULL, separater );
		}
	}

	
	file.close();

	
	is_load_success = true;

	return;

bvh_error:
	file.close();
}




#include <math.h>
#include <gl/glut.h>


static const float DEFAULT_JOINT_COLOR[4] = {0.415f, 0.827f, 0.917f, 1.0f};	// grey-blue

void  BVH::RenderFigure( int frame_no, float scale )
{
    
	glColor4f(DEFAULT_JOINT_COLOR[0],DEFAULT_JOINT_COLOR[1],DEFAULT_JOINT_COLOR[2],DEFAULT_JOINT_COLOR[3]);	
	RenderFigure( joints[ 0 ], motion + frame_no * num_channel, scale );
}



void  BVH::RenderFigure( const Joint * joint, const double * data, float scale )
{
	glPushMatrix();


	if ( joint->parent == NULL )
	{
		glTranslated( data[ 0 ] * scale, data[ 1 ] * scale, data[ 2 ] * scale );
	}
	else
	{
		glTranslated( joint->offset[ 0 ] * scale, joint->offset[ 1 ] * scale, joint->offset[ 2 ] * scale );
	}


	int  i, j;
	for ( i=0; i<(int)joint->channels.size(); i++ )
	{
		Channel *  channel = joint->channels[ i ];
		if ( channel->type == X_ROTATION )
			glRotated( data[ channel->index ], 1.0f, 0.0f, 0.0f );
		else if ( channel->type == Y_ROTATION )
			glRotated( data[ channel->index ], 0.0f, 1.0f, 0.0f );
		else if ( channel->type == Z_ROTATION )
			glRotated( data[ channel->index ], 0.0f, 0.0f, 1.0f );
	}

	
	if ( joint->children.size() == 0 )
	{
		RenderBone( 0.0f, 0.0f, 0.0f, joint->site[ 0 ] * scale, joint->site[ 1 ] * scale, joint->site[ 2 ] * scale );
	}
	
// 	if ( joint->children.size() == 1 )
// 	{
// 		Joint *  child = joint->children[ 0 ];
// 		RenderBone( 0.0f, 0.0f, 0.0f, child->offset[ 0 ] * scale, child->offset[ 1 ] * scale, child->offset[ 2 ] * scale );
// 	}
	
	if ( joint->children.size() >= 1 )
	{
	
// 		double  center[ 3 ] = { 0.0f, 0.0f, 0.0f };
// 		for ( i=0; i<(int)joint->children.size(); i++ )
// 		{
// 			Joint *  child = joint->children[ i ];
// 			center[ 0 ] += child->offset[ 0 ];
// 			center[ 1 ] += child->offset[ 1 ];
// 			center[ 2 ] += child->offset[ 2 ];
// 		}
// 		center[ 0 ] /= joint->children.size() + 1;
// 		center[ 1 ] /= joint->children.size() + 1;
// 		center[ 2 ] /= joint->children.size() + 1;

	
	//	RenderBone(	0.0f, 0.0f, 0.0f, center[ 0 ] * scale, center[ 1 ] * scale, center[ 2 ] * scale );
	//	RenderBone(	0.0f, 0.0f, 0.0f, child->offset[ 0 ] * scale, child->offset[ 1 ] * scale, child->offset[ 2 ] * scale );

		for ( i=0; i<(int)joint->children.size(); i++ )
		{
			

			Joint *  child = joint->children[ i ];

			RenderBone(	0.0f, 0.0f, 0.0f, child->offset[ 0 ] * scale, child->offset[ 1 ] * scale, child->offset[ 2 ] * scale );

		//	RenderBone(	center[ 0 ] * scale, center[ 1 ] * scale, center[ 2 ] * scale,
		//		child->offset[ 0 ] * scale, child->offset[ 1 ] * scale, child->offset[ 2 ] * scale );
		}
	}


	for ( i=0; i<(int)joint->children.size(); i++ )
	{
		RenderFigure( joint->children[ i ], data, scale );
	}

	glPopMatrix();
}



void  BVH::RenderBone( double x0, double y0, double z0, double x1, double y1, double z1 )
{


	GLdouble  dir_x = x1 - x0;
	GLdouble  dir_y = y1 - y0;
	GLdouble  dir_z = z1 - z0;

	double ax = (x0+x1)*0.5f;
	double ay = (y0+y1)*0.5f;
	double az = (z0+z1)*0.5f;


	GLdouble  bone_length = sqrt( dir_x*dir_x + dir_y*dir_y + dir_z*dir_z );

	
	static GLUquadricObj *  quad_obj = NULL;
	if ( quad_obj == NULL )
		quad_obj = gluNewQuadric();

 	gluQuadricDrawStyle( quad_obj, GLU_FILL );
 	gluQuadricNormals( quad_obj, GLU_SMOOTH );

	glPushMatrix();


	glTranslated( x0, y0, z0 );
   // glTranslated( ax, ay, az );


	double  length;
	length = sqrt( dir_x*dir_x + dir_y*dir_y + dir_z*dir_z );
	if ( length < 0.0001 ) { 
		dir_x = 0.0; dir_y = 0.0; dir_z = 1.0;  length = 1.0;
	}
	dir_x /= length;  dir_y /= length;  dir_z /= length;


	GLdouble  up_x, up_y, up_z;
	up_x = 0.0;
	up_y = 1.0;
	up_z = 0.0;


	double  side_x, side_y, side_z;
	side_x = up_y * dir_z - up_z * dir_y;
	side_y = up_z * dir_x - up_x * dir_z;
	side_z = up_x * dir_y - up_y * dir_x;


	length = sqrt( side_x*side_x + side_y*side_y + side_z*side_z );
	if ( length < 0.0001 ) {
		side_x = 1.0; side_y = 0.0; side_z = 0.0;  length = 1.0;
	}
	side_x /= length;  side_y /= length;  side_z /= length;


	up_x = dir_y * side_z - dir_z * side_y;
	up_y = dir_z * side_x - dir_x * side_z;
	up_z = dir_x * side_y - dir_y * side_x;


	GLdouble  m[16] = { side_x, side_y, side_z, 0.0,
	                    up_x,   up_y,   up_z,   0.0,
	                    dir_x,  dir_y,  dir_z,  0.0,
	                    0.0,    0.0,    0.0,    1.0 };
	glMultMatrixd( m );

    GLdouble radius= 0.2f; 
//	GLdouble radius= 0.04; 
	int slices = 8; 
	int stack = 3;  

	gluCylinder( quad_obj, radius, radius, bone_length, slices, stack ); 

	glPopMatrix();
}



// End of BVH.cpp
