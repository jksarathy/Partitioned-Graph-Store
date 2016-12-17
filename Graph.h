#include <map>
#include <set>
#include <list>
#include <vector>
#include <cstdint>
#include <string>

#define SUCCESS 200
#define EXISTS 204
#define ERROR 400 


class Graph {
private:
	std::map<uint64_t, std::set<uint64_t> > my_graph;
public:
	uint64_t numNodes();
	uint64_t numEdges();
	std::vector<uint64_t> getNodes();
	std::vector<std::pair<uint64_t, uint64_t> > getEdges();
	int addNode(uint64_t node_id); 
	int addEdge(uint64_t node_a_id, uint64_t node_b_id);
	int removeNode(uint64_t node_id);
	int removeEdge(uint64_t node_a_id, uint64_t node_b_id);
	std::pair<int, bool> getNode(uint64_t node_id);
	std::pair<int, bool> getEdge(uint64_t node_a_id, uint64_t node_b_id);
	std::pair<int, std::string> getNeighbors(uint64_t node_id);
	std::pair<int, uint64_t> shortestPath(uint64_t node_a_id, uint64_t node_b_id);     
};



