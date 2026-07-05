# NTN-Sim: Routing Engine Design

LEO satellite constellations utilize Inter-Satellite Links (ISLs) to route packets through space before downlinking them to a Gateway ground station. The **RoutingEngine** is responsible for determining the optimal path for packets traversing this network.

## Routing Topologies

1. **Intra-Orbit Links**:
   - Connection between satellites in the same orbital plane (usually static, long-term links).
2. **Inter-Orbit Links**:
   - Connections between satellites in neighboring planes (dynamic, may break near polar regions).
3. **Satellite-to-Gateway Links**:
   - Feeder links down to ground stations connected to the terrestrial core network (gNodeB to UPF).

## Routing Algorithm Interface

The `RoutingEngine` maintains a topology graph of the satellite constellation. It provides path-finding functions based on:
- Shortest-path (Dijkstra/A* algorithms).
- Congestion-aware routing (deflecting packets from overloaded satellites).
- Dynamic-topology routing (handling link disruptions and varying propagation delays).

Future iterations will load topology tables and execute routing callbacks to schedule packet delivery and hop events.
