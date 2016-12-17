#include "Graph.h"

uint64_t Graph::numNodes() {
	return my_graph.size();
}

uint64_t Graph::numEdges() {
	uint64_t numEdges = 0;
	std::map<uint64_t, std::set<uint64_t> >::iterator it;
	for (it = my_graph.begin(); it != my_graph.end(); ++it) {
    	numEdges += (it->second).size();
    }
    return numEdges;
}

std::vector<uint64_t> Graph::getNodes() {
	std::vector<uint64_t> v;
	std::map<uint64_t, std::set<uint64_t> >::iterator it;
	for (it = my_graph.begin(); it != my_graph.end(); ++it) {
		v.push_back(it->first);
	}
	return v;
}

std::vector<std::pair<uint64_t, uint64_t> > Graph::getEdges() {
	std::vector<std::pair<uint64_t, uint64_t> > v;
	std::map<uint64_t, std::set<uint64_t> >::iterator it;
	std::set<uint64_t>::iterator i;
	for (it = my_graph.begin(); it != my_graph.end(); ++it) {
		for (i = my_graph[it->first].begin(); i != my_graph[it->first].end(); ++i) {
			v.push_back(std::make_pair(it->first, *i));
		}
	}
	return v;
}


int Graph::addNode(uint64_t node_id) {
	std::set<uint64_t> new_set;
	if (my_graph.insert(make_pair(node_id, new_set)).second == false) 
		return EXISTS;
	else
		return SUCCESS;
}

int Graph::addEdge(uint64_t node_a_id, uint64_t node_b_id) {     
	if (node_a_id == node_b_id || 
		my_graph.find(node_a_id) == my_graph.end() || 
		my_graph.find(node_b_id) == my_graph.end())         
		return ERROR;     
	else if (my_graph[node_a_id].find(node_b_id) != my_graph[node_a_id].end())
		return EXISTS;
	else {
		my_graph[node_a_id].insert(node_b_id);
		my_graph[node_b_id].insert(node_a_id); 
		return SUCCESS;
	}
}

int Graph::removeNode(uint64_t node_id) {
	if (my_graph.erase(node_id) == 1)
		return SUCCESS;
	else
		return ERROR;
}

int Graph::removeEdge(uint64_t node_a_id, uint64_t node_b_id) {
	if (node_a_id == node_b_id ||
		my_graph.find(node_a_id) == my_graph.end() ||
		my_graph.find(node_b_id) == my_graph.end() ||
		my_graph[node_a_id].find(node_b_id) == my_graph[node_a_id].end() ||
		my_graph[node_b_id].find(node_a_id) == my_graph[node_b_id].end())
		return ERROR;
	else {
		my_graph[node_a_id].erase(node_b_id);
		my_graph[node_b_id].erase(node_a_id);
		return SUCCESS;
	}
}

std::pair<int, bool> Graph::getNode(uint64_t node_id) {
	if (my_graph.find(node_id) == my_graph.end())
		return std::make_pair(SUCCESS, false);
	else
		return std::make_pair(SUCCESS, true);
}

std::pair<int, bool> Graph::getEdge(uint64_t node_a_id, uint64_t node_b_id) {
	if (node_a_id == node_b_id ||
		my_graph.find(node_a_id) == my_graph.end() ||
		my_graph.find(node_b_id) == my_graph.end()) 
		return std::make_pair(ERROR, false);
	else if (my_graph[node_a_id].find(node_b_id) == my_graph[node_a_id].end() ||
			 my_graph[node_b_id].find(node_a_id) == my_graph[node_b_id].end())
		return std::make_pair(SUCCESS, false);
	else
		return std::make_pair(SUCCESS, true);
}

std::pair<int, std::string> Graph::getNeighbors(uint64_t node_id) {
	std::string return_string;
	if (my_graph.find(node_id) == my_graph.end()) {
		return std::make_pair(ERROR, return_string);
	} 
	else {
		std::set<uint64_t>::iterator i;
		for (i = my_graph[node_id].begin(); i != my_graph[node_id].end(); ++i) {
			return_string.append(std::to_string(*i));
			return_string.append(",");
		}
		if (!return_string.empty())
			return_string.pop_back();
		return std::make_pair(SUCCESS, return_string); 
	}
}

std::pair<int, uint64_t> Graph::shortestPath(uint64_t node_a_id, uint64_t node_b_id) {

	uint64_t distance = 0;
    int size = my_graph.size();

    if (node_a_id == node_b_id ||
    	my_graph.find(node_a_id) == my_graph.end() || 
		my_graph.find(node_b_id) == my_graph.end())         
		return std::make_pair(EXISTS, distance); 


    std::map<uint64_t, uint64_t> bfs_map;
    std::map<uint64_t, std::set<uint64_t> >::iterator it;

	for (it = my_graph.begin(); it != my_graph.end(); ++it) {
    	bfs_map[it->first] = size + 1;
    }

    std::list<uint64_t> queue;
    std::set<uint64_t>::iterator i;
 
    bfs_map[node_a_id] = distance;
    queue.push_back(node_a_id);
 
    while (!queue.empty()) {
    	uint64_t current_node = queue.front();
        distance = bfs_map[current_node];
        distance++;
        queue.pop_front();
 
        for (i = my_graph[current_node].begin(); i != my_graph[current_node].end(); ++i) {
        	if (*i == node_b_id) {
        		return std::make_pair(SUCCESS, distance);
        	}
            else if (distance < bfs_map[*i]) {
                bfs_map[*i] = distance;
                queue.push_back(*i);
            }

        }
    }

    return std::make_pair(ERROR, size + 1);
}

