# NTN-Sim: Handover Engine Design

In 5G NTN networks, satellites move at high speeds (e.g., LEO satellites travel at ~7.5 km/s). Consequently, handovers between serving satellite beams occur frequently. The Handover Engine is responsible for managing when and how a User Equipment (UE) changes its serving satellite.

## Architecture

The Handover Engine utilizes the **Strategy Pattern** to swap handover decision-making logic dynamically.

```
       +------------------+
       |  HandoverEngine  |
       +--------+---------+
                |
                v  (uses)
   +------------+------------+
   |    IHandoverAlgorithm   |  <-- Interface
   +------------+------------+
                |
  +-------------+-------------+-------------+-------------+
  |                           |                           |             |
  v                           v                           v             v
+-------------------+       +-------------------+       +-------------+  +---------------+
| BestSignalAlgo    |       | LoadAwareAlgo     |       | Hysteresis  |  | FutureAIAlgo  |
| (Max RSRP)        |       | (Congestion check)|       | (TTT margin)|  | (ML predicted)|
+-------------------+       +-------------------+       +-------------+  +---------------+
```

## Algorithms Supported

1. **BestSignalAlgorithm**:
   - Compares Reference Signal Received Power (RSRP) across visible satellites.
   - Handover is triggered immediately when a neighbor satellite offers a stronger signal than the current serving satellite.

2. **LoadAwareAlgorithm**:
   - Accounts for satellite capacity and user density.
   - Prevents hot-spots by avoiding handovers to satellites with high utilization, balancing network load.

3. **HysteresisAlgorithm**:
   - Implements a handover margin (dB) and Time-to-Trigger (TTT) timer.
   - Prevents the "ping-pong" effect where a user oscillates rapidly between two satellites of comparable signal strength due to channel fading.

4. **FutureAIAlgorithm**:
   - Placeholder for neural network or reinforcement learning-based trajectory prediction handovers.
