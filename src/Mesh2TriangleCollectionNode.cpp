#include "MeshProcessingNodes.hpp"

#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>

namespace geoflow::nodes::stepedge {

  void Mesh2TriangleCollectionNode::process() { 
    
    typedef SurfaceMesh::Vertex_index       VertexIndex;
    typedef SurfaceMesh::Face_index         FaceIndex;
    namespace PMP = CGAL::Polygon_mesh_processing;

    auto smesh = input("cgal_surface_mesh").get<SurfaceMesh>();
    
    if(!CGAL::is_triangle_mesh(smesh)) PMP::triangulate_faces(smesh);
    
    // normal computation:
    // https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2compute_normals_example_8cpp-example.html#a4
    // auto vnormals = smesh.add_property_map<VertexIndex, K::Vector_3>("v:normals", CGAL::NULL_VECTOR).first;
    auto fnormals = smesh.add_property_map<FaceIndex, K::Vector_3>("f:normals", CGAL::NULL_VECTOR).first;
    PMP::compute_face_normals(smesh, fnormals);
    // PMP::compute_normals(smesh, vnormals, fnormals);
    // std::cout << "Vertex normals :" << std::endl;
    // for(VertexIndex vd: vertices(smesh))
    //   std::cout << vnormals[vd] << std::endl;
    // std::cout << "Face normals :" << std::endl;
    // for(FaceIndex fd: faces(smesh))
    //   std::cout << fnormals[fd] << std::endl;
    
    TriangleCollection triangleCollection;
    vec3f normals;
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

    output("triangles").set(triangleCollection);
    output("normals").set(normals);
  }

  void Mesh2CGALSurfaceMeshNode::process() { 
    typedef SurfaceMesh::Vertex_index       VertexIndex;
    namespace PMP = CGAL::Polygon_mesh_processing;

    auto gfmesh = input("mesh").get<Mesh>();
    
    SurfaceMesh smesh;
    {
      std::map<arr3f, VertexIndex> vertex_map;
      std::set<arr3f> vertex_set;
      for (const auto &ring : gfmesh.get_polygons())
      {
        for (auto &v : ring)
        {
          auto [it, did_insert] = vertex_set.insert(v);
          if (did_insert)
          {
            vertex_map[v] = smesh.add_vertex(K::Point_3(v[0],v[1],v[2]));;
          }
        }
      }
    
      for (auto& ring : gfmesh.get_polygons()) {
        std::vector<VertexIndex> rindices;
        rindices.reserve(ring.size());
        for(auto& p : ring) {
          rindices.push_back(vertex_map[p]);
        }
        smesh.add_face(rindices);
      }
    }

    if(!CGAL::is_triangle_mesh(smesh)) PMP::triangulate_faces(smesh);

    output("cgal_surface_mesh").set(smesh);
  }

}