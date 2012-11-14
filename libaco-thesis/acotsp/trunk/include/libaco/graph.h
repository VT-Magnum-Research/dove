#include <vector>
#include <map>
#include <string>
#include <iostream>

template <class T> class Row {
  private:
    std::vector<T> *values_;
  public:
    Row(const Row &nh) {
      values_ = new std::vector<T>(*(nh.values_));
    }

    Row(unsigned int vertices, const T &initialValue) {
      values_ = new std::vector<T>(vertices, initialValue);
    }

    ~Row() {
      delete values_;
    }

    Row &operator=(const Row &nh) {
      *this->values_ = *nh.values_;
      return *this;
    }

    T &operator[](unsigned int i) {
      return (*values_)[i];
    }
};

template <class T> class Matrix {
  protected:
    std::vector< Row<T> > *matrix_;
    unsigned int rows_;
    unsigned int cols_;
  public:
    Matrix(unsigned int rows, unsigned int cols, const T &initialValue) {
      matrix_ = new std::vector< Row<T> >(rows, Row<T> (cols, initialValue));
      rows_ = rows;
      cols_ = cols;
    };

    ~Matrix() {
      delete matrix_;
    }

    Matrix(const Matrix &matrix) {
      matrix_ = new std::vector < Row<T> >(matrix.rows(), Row<T>(matrix.cols(), 0));
      *matrix_ = *(matrix.matrix_);
      rows_ = matrix.rows();
      cols_ = matrix.cols();
    }

    Row<T> &operator[](unsigned int i) {
      return (*matrix_)[i];
    }

    Matrix<T> &operator=(const Matrix<T> &matrix) {
      *this->matrix_ = *matrix.matrix_;
      rows_ = matrix.rows();
      cols_ = matrix.cols();
      return *this;
    }

    unsigned int rows() const {
      return rows_;
    }

    unsigned int cols() const {
      return cols_;
    }
};

class Graph {
  public:
    virtual ~Graph() {}
    virtual void add_edge(unsigned int v, unsigned int w) = 0;
    virtual bool is_edge(unsigned int v, unsigned int w) const = 0;
    virtual void remove_edge(unsigned int v, unsigned int w) = 0;
    virtual unsigned int number_of_vertices() const = 0;
    virtual std::vector<unsigned int> get_neighbours(unsigned int vertex) const = 0;
    virtual unsigned int get_degree(unsigned int vertex) const = 0;
};

class AdjacencyMatrixGraph : public Graph, public Matrix<unsigned short int> {
  public:
    AdjacencyMatrixGraph(unsigned int vertices);
    AdjacencyMatrixGraph(const AdjacencyMatrixGraph &graph);
    AdjacencyMatrixGraph &operator=(const AdjacencyMatrixGraph &graph);
    void add_edge(unsigned int v, unsigned int w);
    bool is_edge(unsigned int v, unsigned int w) const;
    void remove_edge(unsigned int v, unsigned int w);
    unsigned int number_of_vertices() const;
    std::vector<unsigned int> get_neighbours(unsigned int vertex) const;
    unsigned int get_degree(unsigned int vertex) const;
};

template <class T> class AdjacencyMap {
  private:
    std::map<unsigned int, T> values_;
    T default_value_;
  public:
    AdjacencyMap(T default_value) {
      default_value_ = default_value;
    }

    void add_neighbour(unsigned int vertex, T value) {
      values_[vertex] = value;
    }

    void remove_neighbour(unsigned int vertex) {
      values_.erase(vertex);
    }

    AdjacencyMap &operator=(const AdjacencyMap &map) {
      values_ = map.values_;
      return *this;
    }

    T &operator[](unsigned int vertex) {
      typename std::map<unsigned int, T>::iterator it = values_.find(vertex);
      if (it != values_.end()) {
        return (*it).second;
      } else {
        return default_value_;
      }
    }

    unsigned int size() {
      return values_.size();
    }

    std::vector<unsigned int> get_neighbours() {
      std::vector<unsigned int> neighbours;
      typename std::map<unsigned int, T>::iterator it;
      for(it=values_.begin();it!=values_.end();it++) {
        neighbours.push_back((*it).first);
      }
      return neighbours;
    }
};

class AdjacencyMapGraph : public Graph {
  private:
    std::vector< AdjacencyMap<unsigned short int> > *vertices_;
  public:
    AdjacencyMapGraph(unsigned int vertices, unsigned short int default_value=0);
    ~AdjacencyMapGraph();
    AdjacencyMapGraph(const AdjacencyMapGraph &graph);
    AdjacencyMapGraph &operator=(const AdjacencyMapGraph &graph);
    void add_edge(unsigned int v, unsigned int w);
    bool is_edge(unsigned int v, unsigned int w) const;
    void remove_edge(unsigned int v, unsigned int w);
    unsigned int number_of_vertices() const;
    std::vector<unsigned int> get_neighbours(unsigned int vertex) const;
    unsigned int get_degree(unsigned int vertex) const;
};

class HyperGraph {
  private:
    std::map< unsigned int, std::vector<unsigned int> > edges_;
    std::map< unsigned int, std::vector<unsigned int> > vertices_;
    std::map< unsigned int, std::string > edge_labels_;
    std::map< unsigned int, std::string > vertex_labels_;
  public:
    HyperGraph();
    void add_hyperedge(unsigned int edge, std::vector<unsigned int> vertices);
    void set_edge_label(unsigned int edge, std::string label);
    void set_vertex_label(unsigned int vertex, std::string label);
    std::string get_edge_label(unsigned int edge);
    std::string get_vertex_label(unsigned int vertex);
    std::vector<unsigned int> get_edges_for_vertex(unsigned int vertex);
    template <class T> T &get_primal_graph() {
      T *graph;
      graph = new T(vertices_.size());
      for(std::map< unsigned int, std::vector<unsigned int> >::iterator it=edges_.begin();it!=edges_.end();it++) {
        std::vector<unsigned int> *vertices = &(*it).second;
        for(unsigned int i=0;i<vertices->size();i++) {
          for(unsigned int j=i+1;j<vertices->size();j++) {
            graph->add_edge((*vertices)[i], (*vertices)[j]);
          }
        }
      }
      return *graph;
    }
};
