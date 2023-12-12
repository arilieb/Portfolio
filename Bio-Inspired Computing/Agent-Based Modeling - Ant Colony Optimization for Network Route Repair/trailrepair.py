#!/usr/bin/python3
#
# Project:     AntTrailRepair
# Name:        trailrepair.py
# Description: Implementation of turtle ant trail network repair algorithm.
# Author:      Joshua J. Daymude (jdaymude@asu.edu)

import argparse
from collections import deque
from graph_tool.all import *
import math
import numpy as np
import random as rng
import time

class Timer:
    def __init__(self):
        self.last_time = time.time()

    def now(self):
        self.last_time = time.time()

    def lap(self, msg):
        t = time.time()
        print('{}: {:.6f}s'.format(msg, t - self.last_time))
        self.now()

class Ant:
    def __init__(self, node, last_node):
        self.node = node
        self.last_node = last_node
        self.is_backtracking = False

def g_load(fname):
    """
    Creates an undirected graph from the adjacency list in the given file.
    Inputs:  A string filename containing the adjacency list.
    Returns: A graph_tool undirected Graph with a variety of properties.
    """
    # Scan the edge list file to collect the edge information and find the
    # number of vertices in the graph by the maximum node id.
    edge_nums = []
    num_vertices = 0
    with open(fname) as f:
        for line in f:
            edge_num = [int(node_str) for node_str in line.split()]
            edge_nums.append(edge_num)
            num_vertices = max([num_vertices, edge_num[0], edge_num[1]])
    # Next, build the graph.
    G = Graph(directed=False)
    G.vp.nest = G.new_vertex_property('bool')
    G.vp.queue = G.new_vertex_property('vector<int>')
    G.vp.used = G.new_vertex_property('bool')
    G.ep.weight = G.new_edge_property('double')
    G.ep.cut = G.new_edge_property('bool')
    G.add_vertex(num_vertices)
    for edge_num in edge_nums:
        G.add_edge(G.vertex(edge_num[0] - 1), G.vertex(edge_num[1] - 1))
    return G

def g_init_static(G_0, P_len, e_cut, w_0=10):
    """
    Initializes a copy of the input graph so the initial path is the first
    (P_len - 1) edges in the graph, with the (e_cut)-th edge missing. Every edge
    on the initial path is initialized with an initial amount of pheromone.
    Inputs:  A graph_tool undirected Graph to initialize, the integer number of
             vertices in the initial path, the integer index of the edge to cut,
             and a real amount of pheromone to initialize path edges with.
    Returns: A copy of the input graph, initialized.
    """
    G = Graph(G_0)
    G.vp.nest[G.vertex(0)] = True
    G.vp.nest[G.vertex(P_len - 1)] = True
    for i in range(P_len - 1):
        if i + 1 == e_cut:
            G.ep.cut[G.edge(G.vertex(i), G.vertex(i + 1))] = True
        else:
            G.ep.weight[G.edge(G.vertex(i), G.vertex(i + 1))] = w_0
    G.set_edge_filter(G.ep.cut, inverted=True)
    return G, [G.vertex(i) for i in range(P_len)]

def g_init_sample(G_0, size, w_0=10):
    """
    Initializes a random sample of the input graph on the specified number of
    vertices. Sampling is done by selecting a vertex at random and performing a
    breadth-first search from this vertex until the desired number of vertices
    is reached. Two non-adjacent sampled vertices are randomly chosen to be the
    nest vertices, and a random edge in the shortest path between them is cut.
    Inputs:  A graph_tool undirected Graph to initialize, the integer number of
             vertices to sample, and the real amount of pheromone to intialize
             path edges with.
    Returns: A copy of the sampled input graph, initialized.
    """
    G = Graph(G_0)
    vsample = G.new_vertex_property('bool')
    # Do random subgraph sampling, restarting if a small component is found.
    while True:
        start = rng.choice(list(G.vertices()))
        U = deque([start]) + deque(start.all_neighbors())
        num_sampled = 0
        while num_sampled < size:
            if len(U) == 0:  # Not enough vertices in this component.
                for v in G.vertices():
                    vsample[v] = False
                break
            u = U.popleft()
            if not vsample[u]:
                vsample[u] = True
                num_sampled = num_sampled + 1
                U = U + deque([v for v in u.all_neighbors()])
        if num_sampled == size:
            break
    G.set_vertex_filter(vsample)
    # Choose a pair of vertices to be the endpoints of the initial path and
    # remove a random edge on the shortest path between them. Restart if doing
    # so disconnects the sampled subgraph.
    while True:
        nest1 = rng.choice(list(G.vertices()))
        nest2 = rng.choice(list(G.vertices()))
        while nest1 == nest2 and (nest2 in nest1.all_neighbors()):
            nest2 = rng.choice(list(G.vertices()))
        P_short = shortest_path(G, nest1, nest2)
        if len(P_short[1]) != 0:
            cut_edge = rng.choice(P_short[1])
            G.ep.cut[cut_edge] = True
            G.set_edge_filter(G.ep.cut, inverted=True)
            # If this cut disconnected the graph, reset and repeat. Otherwise,
            # set the nests and pheromone and return.
            if len(shortest_path(G, nest1, nest2)[1]) == 0:
                G.ep.cut[cut_edge] = False
                G.set_edge_filter(None)
            else:
                G.vp.nest[nest1] = True
                G.vp.nest[nest2] = True
                for e in P_short[1]:
                    G.ep.weight[e] = w_0 if e != cut_edge else 0
                return G, P_short[0]

def g_draw(G, layout, fname='graph_out.png'):
    """
    Draws the given graph with the specified layout to file.
    Inputs:  A graph_tool Graph to draw, a layout to position the graph with,
             and an optional string filename to save to.
    Returns: None.
    """
    # Set up drawing properties.
    vfill = G.new_vertex_property('vector<double>')
    for v in G.vertices():
        vfill[v] = [0.65, 0, 0, 0.8] if G.vp.nest[v] else [0.1, 0.1, 0.1, 0.8]
    ecolor = G.new_edge_property('vector<double>')
    ewidth = G.new_edge_property('double')
    max_weight = max([G.ep.weight[e] for e in G.edges()])
    edashed = G.new_edge_property('vector<double>')
    dash_len = G.num_vertices() / 160
    for e in G.edges():
        ecolor[e] = [0.18, 0.6, 0.2, 1] if G.ep.weight[e] > 0 \
                                        else [0.2, 0.2, 0.2, 0.8]
        ewidth[e] = 20 * (G.ep.weight[e] / max_weight) + 2
        edashed[e] = [dash_len, dash_len, 0] if G.ep.cut[e] else []
    # Draw graph.
    vertex_props = {'fill_color': vfill, 'size': 13, 'pen_width' : 0}
    edge_props = {'color' : ecolor, 'pen_width' : ewidth, 'dash_style': edashed}
    graph_draw(G, pos=layout, vprops=vertex_props, eprops=edge_props,\
               output_size=(1200, 800), output=fname, bg_color=[1,1,1,1])

def max_paths(G, s, t, last, X):
    """
    Recursive function for computing all maximal paths in the given grapth from
    node s to node t, avoiding the vertices in X.
    Inputs:  A graph_tool Graph G, source and target nodes s and t, the last
             node visited (or None, if not applicable), and a list of vertices
             to avoid.
    Returns: A list of maximal paths from s to t, or None if none exist.
    """
    W = [G.ep.weight[G.edge(s, u)] for u in s.all_neighbors() if u != last]
    if len(W) == 0:
        return None
    U = [u for u in s.all_neighbors() if u not in X and \
         math.isclose(max(W), G.ep.weight[G.edge(s, u)])]
    if len(U) == 0:
        return None
    P = []
    for u in U:
        if u == t:
            return [[s, t]]
        else:
            subpaths = max_paths(G, u, t, s, X + [s])
            if subpaths != None:
                for sub in subpaths:
                    P.append([s] + sub)
    return P if len(P) != 0 else None

if __name__ == '__main__':
    # Read input parameters.
    parser = argparse.ArgumentParser(description='Ant Trail Repair')
    parser.add_argument('-g', '--g_top', default='minimal', help='Graph \
                        topology, one of: [minimal | simple | medium | full | \
                        spanning | euroroad | chicago | uspower | paroad | \
                        caroad]')
    parser.add_argument('-R', '--runs', default=200, help='# of runs')
    parser.add_argument('-T', '--steps', default=1001, help='# of time steps')
    parser.add_argument('-N', '--ants', default=100, help='# of ants')
    parser.add_argument('-C', '--cut', default=6, help='Index of cut edge')
    parser.add_argument('-V', '--size', default=121, help='# of vertices in subgraph samples')
    parser.add_argument('-A', '--add', default=1, help='Amount of pheromone to add on a traveled edge')
    parser.add_argument('-Q', '--decay', default=0.02, help='Fraction of pheromone that evaporates per time step.')
    parser.add_argument('-X', '--explore', default=0.20, help='Probability that an ant takes an explore step')
    parser.add_argument('-E', '--extra', action='store_true', help='Use the variant with extra ants at the nests')
    parser.add_argument('-D', '--draw', action='store_true', help='Draw the trail network at the specified iterations')
    args = parser.parse_args()
    R = int(args.runs)   # Number of runs.
    T = int(args.steps)  # Number of time steps.
    N = int(args.ants)   # Number of ants.
    g_top = args.g_top   # Graph topology.

    # Fixed simulation parameters.
    q_add = int(args.add)           # Amount of pheromone to add on a traveled edge.
    q_decay = float(args.decay)       # Fraction of pheromone that evaporates per time step.
    decay_tol = 1e-10    # Reset threshold for difference after decay.
    q_explore = float(args.explore)     # Probability that an ant takes an explore step.

    # Load the base graph.
    timer = Timer()
    G_0 = g_load('data/g_{}.txt'.format(g_top))
    #timer.lap('Loaded base graph')

    # Perform the experiment runs.
    metrics = {'success' : [], 'entropy' : [], 'length' : [], 'inflate' : []}
    #for r in range(R):
        #print('\nRun {}:'.format(r))
        # Initialize a copy of the base graph.
    if g_top in ['minimal', 'simple', 'medium']:
        G, P_init = g_init_static(G_0, 8, int(args.cut))
    elif g_top in ['full', 'spanning']:
        G, P_init = g_init_static(G_0, 11, int(args.cut))
    elif g_top in ['euroroad', 'chicago', 'uspower', 'paroad', 'caroad']:
        G, P_init = g_init_sample(G_0, int(args.size))
    #timer.lap('\tInitialized g_{} on {} vertices'\
              #.format(g_top, G.num_vertices()))
    draw_layout = None
    # Instance the ants.
    ants = []
    for i in range(N):
        j = rng.randint(0, len(P_init) - 1)
        node = P_init[j]
        last_node = None if G.vp.nest[node] \
                         else rng.choice([P_init[j - 1], P_init[j + 1]])
        ants.append(Ant(node, last_node))
        G.vp.queue[node].append(i)
    # If we're using the extra nest queueing variant, queue the extra ants.
    if args.extra:
        for nest in [P_init[0], P_init[len(P_init) - 1]]:
            for i in range(T):
                ants.append(Ant(nest, None))
                G.vp.queue[nest].append(len(ants) - 1)
    #timer.lap('\tInitialized {} ants'.format(len(ants)))

    # Perform a single experiment run.
    for t in range(T):
        # Draw the graph at the specified iterations.
        if args.draw and t == T - 1:
            G.set_edge_filter(None)
            draw_timer = Timer()
            if draw_layout is None:
                draw_layout = sfdp_layout(G)
                #draw_timer.lap('\tComputed SFDP layout')
            #g_draw(G, draw_layout, 'images/g_{}_r{}.png'.format(g_top, r))
            #draw_timer.lap('\tDrew graph')
            G.set_edge_filter(G.ep.cut, inverted=True)

        # Reset the vertices' queue states.
        for v in G.vertices():
            G.vp.used[v] = False

        # Decay pheromone on all edges.
        for e in G.edges():
            old_weight = G.ep.weight[e]
            G.ep.weight[e] = G.ep.weight[e] * (1 - q_decay)
            if math.isclose(old_weight, G.ep.weight[e], abs_tol=decay_tol):
                G.ep.weight[e] = 0

        # Iterate over all the ants.
        for i in range(N):
            # If ant i is at the front of its node's queue and no one has
            # already left its node in this time step, let it move.
            node_i = ants[i].node
            if (i is G.vp.queue[node_i][0]) and (not G.vp.used[node_i]):
                # If ant i is at a nest or a dead end, it must be allowed to
                # traverse the edge it just came from. Moreover, if it is at
                # a dead end (degree 1), it must backtrack. Backtracking is
                # complete when it is at a node with degree at least 3.
                if G.vp.nest[node_i]:
                    ants[i].last_node = None
                elif node_i.in_degree() + node_i.out_degree() == 1:
                    ants[i].last_node = None
                    ants[i].is_backtracking = True
                if node_i.in_degree() + node_i.out_degree() >= 3:
                    ants[i].is_backtracking = False
                # Next, get the next vertex to go to using RankEdge.
                U = [u for u in node_i.all_neighbors() \
                     if u != ants[i].last_node]
                W = sorted(list(set([G.ep.weight[G.edge(node_i, u)] \
                                     for u in U])), reverse=True)
                next_node = None
                for j in range(len(W)):
                    X = [u for u in U if \
                         math.isclose(G.ep.weight[G.edge(node_i, u)], W[j])]
                    if (j == 0 and len(X) == len(U)) or (j == len(W) - 1):
                        next_node = rng.choice(X)
                        break
                    elif rng.random() < (1 - q_explore):
                        next_node = rng.choice(X)
                        break
                # Move ant i and lay pheromone depending on if this is an
                # explore step and if ant i is backtracking.
                e_taken = G.edge(node_i, next_node)
                w_taken = G.ep.weight[e_taken]
                if math.isclose(w_taken, W[0]):  # No explore.
                    ants[i].last_node = node_i
                    ants[i].node = next_node
                    G.vp.queue[node_i] = np.delete(G.vp.queue[node_i], [0])
                    G.vp.queue[next_node].append(i)
                    if not ants[i].is_backtracking:
                        G.ep.weight[e_taken] = w_taken + q_add
                else:  # Explore step: go and come back.
                    ants[i].last_node = next_node
                    G.vp.queue[node_i] = np.delete(G.vp.queue[node_i], [0])
                    G.vp.queue[node_i].append(i)
                    if not ants[i].is_backtracking:
                        G.ep.weight[e_taken] = w_taken + (2 * q_add)
                # An ant has moved from this node, so mark it as used.
                G.vp.used[node_i] = True
    #timer.lap('\tRan for {} time steps'.format(T))

    # Analyze performance metrics for this run. First, get the shortest path
    # connecting the two nests the algorithm could have found.
    nest1 = P_init[0]
    nest2 = P_init[len(P_init) - 1]
    short = len(shortest_path(G, nest1, nest2)[0])
    # Next, filter down to the pheromone subgraph.
    G.set_edge_filter(None)
    G.set_edge_filter(G.ep.cut, inverted=True)
    pheromone_edge = G.new_edge_property('bool')
    for e in G.edges():
        pheromone_edge[e] = G.ep.weight[e] > 0
    G.set_edge_filter(pheromone_edge)
    # Next, calculate all maximal paths between the two nests and log if
    # this run was successful, i.e., if it found at least one maximal path.
    # Also compute path entropy, average length, and averageinflation of the
    # maximal paths.
    max_Ps = max_paths(G, nest1, nest2, None, [])
    if max_Ps is None:
        success = False
        entropy = None
        ave_len = None
        ave_inf = None
    else:
        stats = []
        for P in max_Ps:
            P_prob = 1
            for i in range(len(P) - 1):
                nbrs = list(P[i].all_neighbors()) if i == 0 else \
                       [u for u in P[i].all_neighbors() if u != P[i-1]]
                w_max = max([G.ep.weight[G.edge(P[i], u)] for u in nbrs])
                maxs = [u for u in nbrs if \
                        math.isclose(w_max, G.ep.weight[G.edge(P[i], u)])]
                P_prob = P_prob * (1 / len(maxs))
            stats.append({'prob' : P_prob, 'length' : len(P)})
        success = True
        entropy = -1 * sum([p['prob'] * math.log(p['prob']) for p in stats])
        ave_len = sum([p['length'] for p in stats]) / len(max_Ps)
        ave_inf = sum([p['length'] / short for p in stats]) / len(max_Ps)
    # Record results.
    metrics['success'].append(success)
    metrics['entropy'].append(entropy)
    metrics['length'].append(ave_len)
    metrics['inflate'].append(ave_inf)
    #timer.lap('\tComputed run stats')

    print(success)
    
    # Print progress.
    #print('\n\tSuccess: {}'.format(success))
    #print('\tEntropy: {}'.format(entropy))
    #print('\tAve. Length: {}'.format(ave_len))
    #3print('\tAve. Inflation: {}'.format(ave_inf))

    # Collate performance metrics.
    #num_succs = len([i for i in metrics['success'] if i])
    #succ_rate = num_succs / len(metrics['success'])
    #ave_entropy = sum([e for e in metrics['entropy'] if e != None]) / num_succs
    #ave_length = sum([l for l in metrics['length'] if l != None]) / num_succs
    #ave_inflate = sum([i for i in metrics['inflate'] if i != None]) / num_succs
    #print('\nResults of {} runs of {} ants for {} steps on g_{}:'\
          #.format(R, N, T, g_top))
    #print('\tSuccess Rate: {}'.format(succ_rate))
    #print('\tAve. Entropy: {}'.format(ave_entropy))
    #print('\tAve. Path Length: {}'.format(ave_length))
    #print('\tAve. Path Inflation: {}'.format(ave_inflate))
