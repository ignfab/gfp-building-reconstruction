#include "MeshProcessingNodes.hpp"

#include <CGAL/Iso_cuboid_3.h>
#include <CGAL/Polygon_mesh_processing/clip.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/intersections.h>

namespace geoflow::nodes::stepedge {

  Triangle create_gf_triangle(const K::Point_3& p0, const K::Point_3& p1, const K::Point_3& p2) {
    Triangle t;
    t[0] = arr3f{ 
      (float) p0.x(),
      (float) p0.y(),
      (float) p0.z()
    };
    t[1] = arr3f{ 
      (float) p1.x(),
      (float) p1.y(),
      (float) p1.z()
    };
    t[2] = arr3f{ 
      (float) p2.x(),
      (float) p2.y(),
      (float) p2.z()
    };
    return t;
  }

  void MeshClipperNode::process() { 
    
    typedef SurfaceMesh::Vertex_index       VertexIndex;
    typedef SurfaceMesh::Face_index         FaceIndex;
    typedef K::Triangle_3                   Triangle_3;
    typedef K::Point_3                      Point_3;
    typedef std::vector<K::Point_3>         Poly_3;
    namespace PMP = CGAL::Polygon_mesh_processing;

    auto smesh = input("mesh").get<SurfaceMesh>();
    
    // SurfaceMesh smesh;
    // {
    //   std::map<arr3f, VertexIndex> vertex_map;
    //   std::set<arr3f> vertex_set;
    //   for (const auto &ring : gfmesh.get_polygons())
    //   {
    //     for (auto &v : ring)
    //     {
    //       auto [it, did_insert] = vertex_set.insert(v);
    //       if (did_insert)
    //       {
    //         vertex_map[v] = smesh.add_vertex(Point_3(v[0],v[1],v[2]));;
    //       }
    //     }
    //   }
    
    //   for (auto& ring : gfmesh.get_polygons()) {
    //     std::vector<VertexIndex> rindices;
    //     rindices.reserve(ring.size());
    //     for(auto& p : ring) {
    //       rindices.push_back(vertex_map[p]);
    //     }
    //     smesh.add_face(rindices);
    //   }
    // }

    // clip
    if(!CGAL::is_triangle_mesh(smesh)) PMP::triangulate_faces(smesh);

    auto& gfbox = input("bbox").get<Box>();
    const auto& pmin = gfbox.min();
    const auto& pmax = gfbox.max();
    K::Iso_cuboid_3 cuboid(
      Point_3(pmin[0], pmin[1], pmin[2]), 
      Point_3(pmax[0], pmax[1], pmax[2])
    );

    TriangleCollection triangleCollection;
    vec3f normals;

    auto fnormals = smesh.add_property_map<FaceIndex, K::Vector_3>("f:normals", CGAL::NULL_VECTOR).first;
    PMP::compute_face_normals(smesh, fnormals);

    if (!skip_clip_) {
      if (cgal_clip_) { // use cgal mesh_processing
        if (!PMP::does_self_intersect(smesh)) {
          PMP::clip(
            smesh,
            cuboid
          );
        }
      } else {
        CGAL::Vertex_around_face_iterator<SurfaceMesh> vit, vend;
        for (auto& f : smesh.faces()) {
          boost::tie(vit, vend) = CGAL::vertices_around_face(smesh.halfedge(f), smesh);
          auto end = vit;
          K::Point_3 p1 = smesh.point(*vit); vit++;
          K::Point_3 p2 = smesh.point(*vit); vit++;
          K::Point_3 p3 = smesh.point(*vit);
          K::Triangle_3 triangle(
            p1,
            p2,
            p3
          );

          const auto result = CGAL::intersection(triangle, cuboid);
          if (result) {
            auto& n = fnormals[f];
            if (const Triangle_3* tri = boost::get<Triangle_3>(&*result)) {
              triangleCollection.push_back(create_gf_triangle(
                tri->vertex(0),
                tri->vertex(1),
                tri->vertex(2)
              ));
              normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
              normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
              normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
            } else if (const Poly_3* poly = boost::get<Poly_3 >(&*result)) {
              // std::cout << "polygon! [" << poly->size() << std::endl;
              
              for(unsigned i=0; i<poly->size()-2; ++i) {
                // std::cout << i << " ";
                triangleCollection.push_back(create_gf_triangle(
                  (*poly)[0],
                  (*poly)[i+1],
                  (*poly)[i+2]
                ));
                normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
                normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
                normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
              }

            }
          }
        }
      }
    }

    if (skip_clip_ || cgal_clip_) {
      for (auto& f : smesh.faces()) {
        Triangle t;
        unsigned i = 0;
        
        for(VertexIndex vi : vertices_around_face(smesh.halfedge(f), smesh)) {
          auto& p = smesh.point(vi);
          t[i++] = arr3f{ 
          (float) p.x(),
          (float) p.y(),
          (float) p.z()
          };
        }
        auto& n = fnormals[f];
        normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
        normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
        normals.push_back(arr3f{ float(n.x()), float(n.y()), float(n.z()) });
        triangleCollection.push_back(t);
      }
    }

    output("triangles").set(triangleCollection);
    output("normals").set(normals);
    // output("cgal_surface_mesh").set(smesh);
  }

}