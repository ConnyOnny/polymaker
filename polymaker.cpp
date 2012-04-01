#include <iostream>
#include <list>

// TWEAK: MKPBM=true -> write pbm file with vertices as black dots on stdout
//        MKPY =true -> write py  file with list of points on stdout
//        both true  -> pbm and py mixed up -> bad idea
#define MKPBM true
#define MKPY false

extern "C" {
	#include "readpng.h"
}
#include <stdlib.h>
#include <limits.h>
#include <vector>

int triang_area (std::pair<int,int> &A, std::pair<int,int> &B, std::pair<int,int> &C) {
	// actually the parallelograme area, but as these values are only used relative to each other, division by 2 is not needed
	int res = (A.first-B.first)*(A.second-C.second)-(A.second-B.second)*(A.first-C.first);
	//TWEAK: CONVEX defined -> triangles pointing inside have negative are and are always removed
	#define CONVEX
	#ifdef CONVEX
	return res;
	#else
	return res<0?-res:res;
	#endif
	#undef CONVEX
}

std::pair<int,int> findFirst (unsigned char ** arr, size_t w, size_t h) {
	for (size_t y=0; y<h; y++)
		for (size_t x=0; x<w; x++)
			if (arr[x][y])
				return std::make_pair<int,int>(x,y);
}

std::list<std::pair<int,int> > getPolygon (std::pair<int,int> P1, std::pair<int,int> P2, unsigned char ** arr, int w, int h) {
	std::list<std::pair<int,int> > ret;
	std::pair<int,int> firstPoint = P1;
	std::pair<int,int> P0 = std::make_pair<int,int> (-10,-10);
	ret.push_back(P1);
	ret.push_back(P2);
	std::list<std::pair<int,int> > checkList; // this order is needed to not run in a 3-pixel circle
	checkList.push_back (std::make_pair<int,int>(-1,0));
	checkList.push_back (std::make_pair<int,int>(1,0));
	checkList.push_back (std::make_pair<int,int>(0,1));
	checkList.push_back (std::make_pair<int,int>(0,-1));
	checkList.push_back (std::make_pair<int,int>(-1,-1));
	checkList.push_back (std::make_pair<int,int>(-1,1));
	checkList.push_back (std::make_pair<int,int>(1,-1));
	checkList.push_back (std::make_pair<int,int>(1,1));
	next:
	for (std::list<std::pair<int,int> >::const_iterator checkIt = checkList.begin(); checkIt != checkList.end(); ++checkIt) {
		int dx = checkIt->first;
		int dy = checkIt->second;
		int newx = P2.first+dx;
		int newy = P2.second+dy;
		//std::cout << "check: (" << newx << "," << newy << ")\n";
		if (newx == P1.first && newy == P1.second) // old point
			continue;
		//std::cout << ".";
		if (newx == P0.first && newy == P0.second) // old point
			continue;
		//std::cout << ".";
		if (newx < 0 || newx >= w || newy < 0 || newy >= h) // out of canvas
			continue;
		//std::cout << ".";
		if (newx == firstPoint.first && newy == firstPoint.second)
			return ret;
		//std::cout << ".";
		if (arr[newx][newy]) {
			P0 = P1;
			P1 = P2;
			P2 = std::make_pair<int,int>(newx,newy);
			ret.push_back(P2);
			//std::cout << "!\n";
			goto next;
		}
		//std::cout << "\n";
	}
	std::cerr << "ERROR!" << std::endl;
	return ret;
}

int main (int argc, char** argv) {
	int threashold = 127; // TWEAK: the minimal alpha value to be considered opaque
	int maxrem = INT_MAX; // TWEAK: how large may a triangle at most be to be removed, INT_MAX -> as large as you want, use only vertices variable below
	int vertices = 8; // TWEAK: how many vertices should there be in the end, 0 -> use only maxrem
	bool clockwise = true; // TODO: see below, counter-clockwise is not implemented yet but should be easy
	int imgwidth, imgheight;
	unsigned char ** alpha_data;
	alpha_data = get_alpha(argv[1], &imgwidth, &imgheight); // argv[1] should contain the png file name
	if (!alpha_data) {
		std::cerr << "png read error" << std::endl;
		return 1;
	}
	unsigned char **edge_data = (unsigned char **) malloc (imgwidth*sizeof(unsigned char*));
	for (int x=0; x<imgwidth; x++) {
		edge_data[x] = (unsigned char*) malloc (imgheight*sizeof(unsigned char));
	}
	//std::cout << "P1\n" << imgwidth << " " << imgheight << "\n";
	for (int y=0; y<imgheight; y++) {
		for (int x=0; x<imgwidth; x++) {
			bool aroundinvis;
			if (y==0 || y==imgheight-1 || x==0 || x==imgwidth-1)
				aroundinvis=true;
			else
				aroundinvis = alpha_data[x-1][y] <= threashold || alpha_data[x+1][y] <= threashold || alpha_data[x][y-1] <= threashold || alpha_data[x][y+1] <= threashold;
			edge_data[x][y] = alpha_data[x][y] > threashold && aroundinvis;
			//std::cout << (edge_data[x][y] ? "1" : "0");
		}
		//std::cout << std::endl;
	}
	std::pair<int,int> firstPoint = findFirst (edge_data, imgwidth, imgheight);
	std::pair<int,int> secondPoint = std::make_pair<int,int>(-1,-1);
	if (clockwise) {
		if (edge_data[firstPoint.first+1][firstPoint.second])
			secondPoint = std::make_pair<int,int>(firstPoint.first+1,firstPoint.second);
		else if (edge_data[firstPoint.first+1][firstPoint.second+1])
			secondPoint = std::make_pair<int,int>(firstPoint.first+1,firstPoint.second);
		else {
			std::cerr << "ERROR!";
			return 1;
		}
	} // TODO: else
	std::list<std::pair<int,int> > polygon = getPolygon (firstPoint, secondPoint, edge_data, imgwidth, imgheight);
	std::vector<std::pair<int,int> > P (polygon.begin(), polygon.end());

	int index = -1;
	do { // TODO: cache the calculated triangle areas for better performance on larger images (amortized linear complexity instead of quadratic w.r.t. the number of edge pixels)
		int least = INT_MAX;
		index = -1;
		for (int i=0; i<(int)P.size(); i++) {
			int lindex = i == 0 ? P.size()-1 : i-1;
			int rindex = i == P.size()-1 ? 0 : i+1;
			int here = triang_area(P[lindex],P[i],P[rindex]);
			if (here < least && here <= maxrem) {
				index = i;
				least = here;
			}
		}
		if (index>=0 && !P.empty())
			P.erase(P.begin()+index);
	} while (index >= 0 && P.size() > vertices);
	if (MKPY) {
		std::cout << "#!/usr/bin/env python" << std::endl << std::endl;
		std::cout << "# Polygon created with polymaker " << __DATE__ << " for image file \"" << argv[1] << "\"" << std::endl;
		std::cout << "# Usually you don't want to execute this file." << std::endl;
		std::cout << "# The preferred usage for polymaker's .py files is this: You do in your code somewhere" << std::endl;
		std::cout << "#  from thepolyfile.py import polygon as myobjectspolygon" << std::endl;
		std::cout << "# If you execute this file, it will print your polygon as list of pairs, beginning with a single number telling its number of vertices" << std::endl << std::endl;
		#ifdef PYPOLYHQ
		std::cout << "polygon_hq = [";
		for (std::list<std::pair<int,int> >::const_iterator iterator = polygon.begin(); iterator != polygon.end(); ++iterator) {
		    std::cout << "(" << iterator->first << "," << iterator->second << "), ";
		}
		std::cout << "]" << std::endl;
		#endif
		std::cout << "polygon = [";
		for (std::vector<std::pair<int,int> >::const_iterator iterator = P.begin(); iterator != P.end(); ++iterator) {
		    std::cout << "(" << iterator->first << "," << iterator->second << "), ";
		}
		std::cout << "]" << std::endl << std::endl;
		std::cout << "#print (len(polygon))" << std::endl << "#for vertex in polygon:" << std::endl << "#\tprint (vertex)"<<std::endl;
	}
	if (MKPBM) {
		unsigned char **edge_databl = (unsigned char **) calloc (imgwidth,sizeof(unsigned char*));
		for (int x=0; x<imgwidth; x++) {
			edge_databl[x] = (unsigned char*) calloc (imgheight,sizeof(unsigned char));
		}
		for (std::vector<std::pair<int,int> >::const_iterator iterator = P.begin(); iterator != P.end(); ++iterator) {
		    edge_databl[iterator->first][iterator->second] = 1;
		}
		std::cout << "P1\n" << imgwidth << " " << imgheight << "\n";
		for (int y=0; y<imgheight; y++) {
			for (int x=0; x<imgwidth; x++) {
				std::cout << (edge_databl[x][y] ? "1" : "0");
			}
			std::cout << std::endl;
		}
	}

	return 0;
}

