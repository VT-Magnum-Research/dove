#include <string>
#include <cstdlib>
#include <cmath>
#include <acotsp/tsp.h>
#include <liblocalsearch/localsearch.h>

bool debug = true;

TspProblem::TspProblem(Matrix<unsigned int> *distances) {
    if (debug)
        std::cout << "TspProblem::TspProblem" << std::endl;
    distances_ = distances;
}

TspProblem::~TspProblem() {
    if (debug)
        std::cout << "TspProblem::~TspProblem" << std::endl;
    delete distances_;
}

unsigned int TspProblem::get_max_tour_size() {
    if (debug)
        std::cout << "TspProblem::get_max_tour_size" << std::endl;
    return distances_->rows() + 1;
}

unsigned int TspProblem::number_of_vertices() {
    if (debug)
        std::cout << "TspProblem::number_of_vertices" << std::endl;
    return distances_->rows();
}

std::map<unsigned int,double> TspProblem::get_feasible_start_vertices() {
    if (debug)
        std::cout << "TspProblem::get_feasible_start_vertices" << std::endl;
    std::map<unsigned int,double> vertices;
    start_vertex_ = Util::random_number(distances_->rows());
    vertices[start_vertex_] = 1.0;
    return vertices;
}

std::map<unsigned int,double> TspProblem::get_feasible_neighbours(unsigned int vertex) {
    if (debug)
        std::cout << "TspProblem::get_feasible_neighbors: " << vertex << std::endl;
    std::map<unsigned int,double> vertices;
    for(unsigned int i=0;i<distances_->cols();i++) {
        if (!visited_vertices_[i]) {
            unsigned int distance = (*distances_)[vertex][i];
            vertices[i] = 1.0 / (distance + 1.0);
        }
    }
    
    if(vertices.size() == 0) {
        vertices[start_vertex_] = 1.0;
    }
    return vertices;
}

double TspProblem::eval_tour(const std::vector<unsigned int> &tour) {
    if (debug)
        std::cout << "TspProblem::eval_tour" << std::endl;
    double length = 0.0;
    for(unsigned int i=1;i<tour.size();i++) {
        length += (*distances_)[tour[i-1]][tour[i]];
    }
    return length;
}

double TspProblem::pheromone_update(unsigned int v, double tour_length) {
    if (debug)
        std::cout << "TspProblem::pheromone_update: " << v << ", " << tour_length << std::endl;
    return 1.0 / tour_length;
}

void TspProblem::added_vertex_to_tour(unsigned int vertex) {
    if (debug)
        std::cout << "TspProblem::added_vertex_to_tour: " << vertex << std::endl;
    visited_vertices_[vertex] = true;
}

bool TspProblem::is_tour_complete(const std::vector<unsigned int> &tour) {
    if (debug)
        std::cout << "TspProblem::is_tour_complete" << std::endl;
    return tour.size() == (distances_->rows() + 1);
}

std::vector<unsigned int> TspProblem::apply_local_search(const std::vector<unsigned int> &tour) {
    if (debug)
        std::cout << "TspProblem::apply_local_search" << std::endl;
    return tour;
}

void TspProblem::cleanup() {
    if (debug)
        std::cout << "TspProblem::cleanup" << std::endl;
    visited_vertices_.clear();
}

FileNotFoundException::FileNotFoundException(const char *filepath) {
    filepath_ = filepath;
}

const char *FileNotFoundException::what() const throw() {
    return filepath_;
}

Matrix<unsigned int> *Parser::parse_tsplib(const char *filepath) throw(FileNotFoundException) {
    enum section {
        TYPE,
        DIMENSION,
        EDGE_WEIGHT_TYPE,
        NODE_COORD_SECTION,
        NONE
    };
    
    Matrix<unsigned int> *distances;
    std::vector<city> cities;
    section s = NONE;
    std::string keyword;
    std::ifstream file(filepath);
    
    if(!file) {
        throw FileNotFoundException(filepath);
    }
    
    while(file.good()) {
        file >> keyword;
        if(keyword.find("DIMENSION") != std::string::npos) {
            s = DIMENSION;
            if(keyword.length() == std::string("DIMENSION").length()) {
                file >> keyword; // read colon
            }
        } else if(keyword.find("NODE_COORD_SECTION") != std::string::npos) {
            s = NODE_COORD_SECTION;
        }
        
        if (s == DIMENSION) {
            int number_of_vertices;
            file >> number_of_vertices;
            distances = new Matrix<unsigned int>(number_of_vertices, number_of_vertices, 0);
        } else if (s == NODE_COORD_SECTION) {
            unsigned int i=0;
            while(i < distances->rows()) {
                city c;
                file >> c.num >> c.coord_x >> c.coord_y;
                cities.push_back(c);
                i++;
            }
            
            for(unsigned int j=0;j<cities.size();j++) {
                for(unsigned int k=j+1;k<cities.size();k++) {
                    city a = cities[j];
                    city b = cities[k];
                    double xd = a.coord_x - b.coord_x;
                    double yd = a.coord_y - b.coord_y;
                    unsigned int dab = int(round(sqrt(xd*xd + yd*yd)));
                    (*distances)[a.num-1][b.num-1] = dab;
                    (*distances)[b.num-1][a.num-1] = dab;
                }
            }
        }
        s = NONE;
    }
    file.close();
    return distances;
}
