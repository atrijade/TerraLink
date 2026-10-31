import React from 'react';
import {
  Globe,
  Radio,
  Eye,
  EyeOff,
  ZoomIn,
  ZoomOut,
  Maximize2,
  Share2,
} from 'lucide-react';

export interface GlobeLayerState {
  showOrbits: boolean;
  showIsl: boolean;
  showFeeders: boolean;
  showAccess: boolean;
  showGroundStations: boolean;
}

interface GlobeControlsProps {
  layers: GlobeLayerState;
  onToggleLayer: (key: keyof GlobeLayerState) => void;
  onResetCamera: () => void;
  onZoomIn: () => void;
  onZoomOut: () => void;
  onFocusEntity?: (entityId: string) => void;
}

export const GlobeControls: React.FC<GlobeControlsProps> = ({
  layers,
  onToggleLayer,
  onResetCamera,
  onZoomIn,
  onZoomOut,
}) => {
  return (
    <div className="absolute top-4 right-4 z-20 flex flex-col gap-2">
      {/* Camera Controls */}
      <div className="bg-slate-900/80 backdrop-blur-md border border-slate-700/60 rounded-lg p-1.5 shadow-xl flex flex-col gap-1">
        <button
          onClick={onZoomIn}
          title="Zoom In"
          className="p-2 hover:bg-slate-800 text-slate-300 hover:text-cyan-400 rounded transition"
        >
          <ZoomIn size={16} />
        </button>
        <button
          onClick={onZoomOut}
          title="Zoom Out"
          className="p-2 hover:bg-slate-800 text-slate-300 hover:text-cyan-400 rounded transition"
        >
          <ZoomOut size={16} />
        </button>
        <button
          onClick={onResetCamera}
          title="Reset Global View"
          className="p-2 hover:bg-slate-800 text-slate-300 hover:text-cyan-400 rounded transition"
        >
          <Maximize2 size={16} />
        </button>
      </div>

      {/* Layer Toggles */}
      <div className="bg-slate-900/80 backdrop-blur-md border border-slate-700/60 rounded-lg p-2 shadow-xl flex flex-col gap-1.5 text-xs">
        <div className="text-[10px] font-semibold tracking-wider uppercase text-slate-400 px-1 mb-0.5">
          Layers
        </div>

        <button
          onClick={() => onToggleLayer('showOrbits')}
          className={`flex items-center justify-between gap-2 px-2 py-1.5 rounded transition ${
            layers.showOrbits
              ? 'bg-cyan-950/60 text-cyan-300 border border-cyan-800/50'
              : 'text-slate-400 hover:bg-slate-800/60'
          }`}
        >
          <div className="flex items-center gap-1.5">
            <Globe size={13} className="text-cyan-400" />
            <span>Orbits</span>
          </div>
          {layers.showOrbits ? <Eye size={12} /> : <EyeOff size={12} />}
        </button>

        <button
          onClick={() => onToggleLayer('showIsl')}
          className={`flex items-center justify-between gap-2 px-2 py-1.5 rounded transition ${
            layers.showIsl
              ? 'bg-cyan-950/60 text-cyan-300 border border-cyan-800/50'
              : 'text-slate-400 hover:bg-slate-800/60'
          }`}
        >
          <div className="flex items-center gap-1.5">
            <Share2 size={13} className="text-cyan-400" />
            <span>ISL Beams</span>
          </div>
          {layers.showIsl ? <Eye size={12} /> : <EyeOff size={12} />}
        </button>

        <button
          onClick={() => onToggleLayer('showFeeders')}
          className={`flex items-center justify-between gap-2 px-2 py-1.5 rounded transition ${
            layers.showFeeders
              ? 'bg-amber-950/60 text-amber-300 border border-amber-800/50'
              : 'text-slate-400 hover:bg-slate-800/60'
          }`}
        >
          <div className="flex items-center gap-1.5">
            <Radio size={13} className="text-amber-400" />
            <span>Feeders</span>
          </div>
          {layers.showFeeders ? <Eye size={12} /> : <EyeOff size={12} />}
        </button>

        <button
          onClick={() => onToggleLayer('showAccess')}
          className={`flex items-center justify-between gap-2 px-2 py-1.5 rounded transition ${
            layers.showAccess
              ? 'bg-blue-950/60 text-blue-300 border border-blue-800/50'
              : 'text-slate-400 hover:bg-slate-800/60'
          }`}
        >
          <div className="flex items-center gap-1.5">
            <div className="w-2.5 h-0.5 bg-blue-400 rounded-full" />
            <span>Access</span>
          </div>
          {layers.showAccess ? <Eye size={12} /> : <EyeOff size={12} />}
        </button>
      </div>
    </div>
  );
};
