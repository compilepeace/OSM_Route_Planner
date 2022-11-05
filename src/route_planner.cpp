#include "route_planner.h"
#include <algorithm>

/*
    Constructor for RoutePlanner
        Initialize the start & end node from user-supplied input coordinates.
*/
RoutePlanner::RoutePlanner(RouteModel &model, float start_x, float start_y, float end_x, float end_y): m_Model(model)
{
    // Convert input coordinates to percentage
    start_x *= 0.01;
    start_y *= 0.01;
    end_x *= 0.01;
    end_y *= 0.01;

    this->start_node = &m_Model.FindClosestNode(start_x, start_y);
    this->end_node = &m_Model.FindClosestNode(end_x, end_y); 
}


/*
    HValue of a node is 'its distance to the end_node'. Takes care of the 'direction' factor while routing.
*/
float RoutePlanner::CalculateHValue(RouteModel::Node const *node)
{
    return node->distance(*end_node);
}


/*
    AddNeighbors() adds all unvisited neighbors of current node to the open_list.
*/
void RoutePlanner::AddNeighbors(RouteModel::Node *current_node)
{

    // retrieve current node's neighbors
    current_node->FindNeighbors();

    // For each neighboring node, set its attributes appropriately.
    for (auto node: current_node->neighbors){
        node->parent = current_node; 
        node->h_value = CalculateHValue(node); 
        node->g_value = current_node->g_value + node->distance(*current_node);
        open_list.emplace_back(node);
        node->visited = true;
    }
}


/*
    NextNode()
        sorts the open_list and returns the 'cheapest node' in terms of cost which
        is determined by f_value, i.e. g_value (distance) + h_value (direction) cost. 
        The node to be returned is poped off from open_list.
    comparator()
        used to guide std::sort() to arrange nodes in descending order of their
        cost (i.e. h_value + g_value). Descending order is choosed because nodes are
        stored in open_list (vector) for which we have pop_back() operation to 
        extract the node from the end; therefore, the cheapest node should be present
        at the end of vector.
*/
static bool comparator(const RouteModel::Node *a, const RouteModel::Node *b)
{
  auto aValue = a->g_value + a->h_value;
  auto bValue = b->g_value + b->h_value;
  return (aValue > bValue);
}
RouteModel::Node *RoutePlanner::NextNode() 
{
  // write a comparator function ?
  sort(open_list.begin(), open_list.end(), comparator);
  auto resultNode = open_list.back();
  open_list.pop_back();
  return resultNode;
}

/*
    Compute total distance of route and construct the final path as determined by
    A* search algorithm.
*/
std::vector<RouteModel::Node> RoutePlanner::ConstructFinalPath(RouteModel::Node *current_node)
{
    // Create path_found vector and initialize total distance to 0.0f
    std::vector<RouteModel::Node> path_found;
    distance = 0.0f;
    
    // computing total distance from start to end node and
    // populate path_found vector with all nodes along the way
    // (iterating from end node until reaching the start node).
    RouteModel::Node *parser = current_node;
    while (parser->parent != nullptr){
        path_found.emplace_back(*parser);
        distance += parser->distance(*(parser->parent));     
        parser = parser->parent;
    }
    path_found.emplace_back(*parser);

    // reverse the vector O(n) since path_found vector is constructed in reverse order.
    std::reverse(path_found.begin(), path_found.end());     

    distance *= m_Model.MetricScale();  // Multiply the distance by the scale of the map to get meters.
    return path_found;
}


/*
    Implements the A* search algorithm
        searches for shortest route between start & end nodes, constructs the final path
        and stores it in RoutePlanner::m_Model.path (which is later displayed on map tile)
*/

void RoutePlanner::AStarSearch() 
{
    RouteModel::Node *current_node = nullptr;

    // initialize open_list by adding start_node and marking it as visited
    start_node->visited = true;
    open_list.emplace_back(start_node);

    // get the cheapest node from open_list (via NextNode()) called current_node.
    // check if current_node is the end node we want to reach.
    // If yes -> ConstructFinalPath and return
    // else   -> expand current_node to add its neighbors to the open_list. 
    // loop until there is no more node left in open_list to explore.
    while (open_list.size() > 0){
        current_node = NextNode();
        if (current_node == end_node){
            m_Model.path = ConstructFinalPath(current_node);
            return;
        }
        else {
            AddNeighbors(current_node);        
        }
    };
}