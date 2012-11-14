#include <libaco/graph.h>

AdjacencyMatrixGraph::AdjacencyMatrixGraph(unsigned int vertices) : Matrix<unsigned short int>(vertices, vertices, 0) {
}

AdjacencyMatrixGraph::AdjacencyMatrixGraph(const AdjacencyMatrixGraph &graph) : Matrix<unsigned short int>(graph) {
}

AdjacencyMatrixGraph &AdjacencyMatrixGraph::operator=(const AdjacencyMatrixGraph &graph) {
  Matrix<unsigned short int>::operator=(graph);
  return *this;
}

unsigned int AdjacencyMatrixGraph::number_of_vertices() const {
  return matrix_->size();
}

std::vector<unsigned int> AdjacencyMatrixGraph::get_neighbours(unsigned int vertex) const {
  std::vector<unsigned int> neighbours;
  for(unsigned int i=0;i<number_of_vertices();i++) {
    if((*matrix_)[vertex][i]) {
      neighbours.push_back(i);
    }
  }
  return neighbours;
}

void AdjacencyMatrixGraph::add_edge(unsigned int v, unsigned int w) {
  (*matrix_)[v][w] = 1;
  (*matrix_)[w][v] = 1;
}

bool AdjacencyMatrixGraph::is_edge(unsigned int v, unsigned int w) const {
  return (*matrix_)[v][w];
}

void AdjacencyMatrixGraph::remove_edge(unsigned int v, unsigned int w) {
  (*matrix_)[v][w] = 0;
  (*matrix_)[w][v] = 0;
}

unsigned int AdjacencyMatrixGraph::get_degree(unsigned int vertex) const {
  unsigned int degree=0;
  for(unsigned int i=0;i<(*matrix_).size();i++) {
    if((*matrix_)[vertex][i]) {
      degree+=1;
    }
  }
  return degree;
}

AdjacencyMapGraph::AdjacencyMapGraph(unsigned int vertices, unsigned short int default_value) {
  vertices_ = new std::vector< AdjacencyMap<unsigned short int> >(vertices, AdjacencyMap<unsigned short int>(default_value));
}

AdjacencyMapGraph::~AdjacencyMapGraph() {
  delete vertices_;
}

AdjacencyMapGraph::AdjacencyMapGraph(const AdjacencyMapGraph &graph) {
  vertices_ = new std::vector< AdjacencyMap<unsigned short int> >(graph.number_of_vertices(), AdjacencyMap<unsigned short int>(0));
  *vertices_ = *graph.vertices_;
}

AdjacencyMapGraph &AdjacencyMapGraph::operator=(const AdjacencyMapGraph &graph) {
  *vertices_ = *graph.vertices_;
  return *this;
}

void AdjacencyMapGraph::add_edge(unsigned int v, unsigned int w) {
  (*vertices_)[v].add_neighbour(w, 1);
  (*vertices_)[w].add_neighbour(v, 1);
}

bool AdjacencyMapGraph::is_edge(unsigned int v, unsigned int w) const {
  return (*vertices_)[v][w];
}

void AdjacencyMapGraph::remove_edge(unsigned int v, unsigned int w) {
  (*vertices_)[v].remove_neighbour(w);
  (*vertices_)[w].remove_neighbour(v);
}

unsigned int AdjacencyMapGraph::number_of_vertices() const {
  return vertices_->size();
}

std::vector<unsigned int> AdjacencyMapGraph::get_neighbours(unsigned int vertex) const {
  return (*vertices_)[vertex].get_neighbours();
}

unsigned int AdjacencyMapGraph::get_degree(unsigned int vertex) const {
  return (*vertices_)[vertex].size();
}

HyperGraph::HyperGraph() {
}

void HyperGraph::add_hyperedge(unsigned int edge, std::vector<unsigned int> vertices) {
  edges_[edge] = vertices;
  for(unsigned int i=0;i<vertices.size();i++) {
    vertices_[vertices[i]].push_back(edge);
  }
}

void HyperGraph::set_edge_label(unsigned int edge, std::string label) {
  edge_labels_[edge] = label;
}

void HyperGraph::set_vertex_label(unsigned int vertex, std::string label) {
  vertex_labels_[vertex] = label;
}

std::string HyperGraph::get_edge_label(unsigned int edge) {
  return edge_labels_[edge];
}

std::string HyperGraph::get_vertex_label(unsigned int vertex) {
  return vertex_labels_[vertex];
}

std::vector<unsigned int> HyperGraph::get_edges_for_vertex(unsigned int vertex) {
  return vertices_[vertex];
}
