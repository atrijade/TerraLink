import React, { useEffect, useRef, useState, useCallback } from 'react';
import * as Cesium from 'cesium';
import type {
  Satellite,
  TopologyLink,
  Gateway,
  User,
} from '../../api/types';
import {
  geodeticToCartesian,
  ecefToCartesian,
  generateOrbitalTrackPositions,
  createHealthyIslMaterial,
  createFailedLinkMaterial,
  createFeederLinkMaterial,
  createAccessLinkMaterial,
} from './cesiumUtils';
import { GlobeControls, type GlobeLayerState } from './GlobeControls';

interface GlobeViewerProps {
  satellites: Satellite[];
  topology: TopologyLink[];
  gateways: Gateway[];
  users: User[];
  selectedEntityId: string | null;
  onSelectEntity: (entityId: string | null) => void;
  layers?: GlobeLayerState;
  onToggleLayer?: (key: keyof GlobeLayerState) => void;
}

export const GlobeViewer: React.FC<GlobeViewerProps> = ({
  satellites,
  topology,
  gateways,
  users,
  selectedEntityId,
  onSelectEntity,
  layers: externalLayers,
  onToggleLayer: externalToggleLayer,
}) => {
  const containerRef = useRef<HTMLDivElement>(null);
  const viewerRef = useRef<Cesium.Viewer | null>(null);
  const entitiesRef = useRef<{
    satellites: Map<string, Cesium.Entity>;
    links: Map<string, Cesium.Entity>;
    feeders: Map<string, Cesium.Entity>;
    access: Map<string, Cesium.Entity>;
    gateways: Map<string, Cesium.Entity>;
    users: Map<string, Cesium.Entity>;
    orbits: Cesium.Entity[];
  }>({
    satellites: new Map(),
    links: new Map(),
    feeders: new Map(),
    access: new Map(),
    gateways: new Map(),
    users: new Map(),
    orbits: [],
  });

  const [internalLayers, setInternalLayers] = useState<GlobeLayerState>({
    showSatellites: true,
    showLabels: true,
    showOrbits: true,
    showIsl: true,
    showFeeders: true,
    showAccess: true,
    showGroundStations: true,
  });

  const activeLayers = externalLayers || internalLayers;
  const toggleLayer = externalToggleLayer || ((key: keyof GlobeLayerState) => {
    setInternalLayers((prev) => ({ ...prev, [key]: !prev[key] }));
  });

  // =========================================================================
  // Initialize Cesium Viewer
  // =========================================================================
  useEffect(() => {
    if (!containerRef.current) return;

    // Use bundled NaturalEarthII imagery (100% keyless, offline-capable, no watermarks)
    const baseLayer = Cesium.ImageryLayer.fromProviderAsync(
      Cesium.TileMapServiceImageryProvider.fromUrl(
        Cesium.buildModuleUrl('Assets/Textures/NaturalEarthII')
      )
    );

    const viewer = new Cesium.Viewer(containerRef.current, {
      animation: false,
      baseLayerPicker: false,
      fullscreenButton: false,
      geocoder: false,
      homeButton: false,
      infoBox: false,
      sceneModePicker: false,
      selectionIndicator: false,
      timeline: false,
      navigationHelpButton: false,
      baseLayer: baseLayer,
      skyAtmosphere: new Cesium.SkyAtmosphere(),
    });

    viewerRef.current = viewer;

    // Configure globe visual properties
    viewer.scene.globe.enableLighting = true;
    viewer.scene.globe.showGroundAtmosphere = true;
    viewer.scene.screenSpaceCameraController.minimumZoomDistance = 300000; // 300 km
    viewer.scene.screenSpaceCameraController.maximumZoomDistance = 40000000; // 40,000 km

    // Initial camera position overlooking global constellation
    viewer.camera.flyTo({
      destination: Cesium.Cartesian3.fromDegrees(10.0, 30.0, 22000000.0),
      duration: 0.0,
    });

    // Generate static Walker constellation orbital tracks
    const orbitPlanes = 4;
    for (let p = 0; p < orbitPlanes; p++) {
      const raanDeg = p * (360.0 / orbitPlanes);
      const trackPoints = generateOrbitalTrackPositions(raanDeg, 53.0, 600.0, 100);

      const orbitLine = viewer.entities.add({
        name: `Orbit Plane ${p}`,
        polyline: {
          positions: trackPoints,
          width: 1.2,
          material: new Cesium.PolylineDashMaterialProperty({
            color: Cesium.Color.fromCssColorString('rgba(250, 204, 21, 0.45)'),
            dashLength: 12.0,
          }),
        },
      });
      entitiesRef.current.orbits.push(orbitLine);
    }

    // Entity click handler
    const handler = new Cesium.ScreenSpaceEventHandler(viewer.scene.canvas);
    handler.setInputAction((click: { position: Cesium.Cartesian2 }) => {
      const pickedObject = viewer.scene.pick(click.position);
      if (Cesium.defined(pickedObject) && pickedObject.id) {
        const id = pickedObject.id.name || pickedObject.id.id;
        if (id) {
          const cleanId = id.replace(/^(sat_|gw_|ue_)/, '');
          onSelectEntity(cleanId);
          return;
        }
      }
      onSelectEntity(null);
    }, Cesium.ScreenSpaceEventType.LEFT_CLICK);

    return () => {
      handler.destroy();
      viewer.destroy();
      viewerRef.current = null;
    };
  }, [onSelectEntity]);

  // =========================================================================
  // Layer Visibility Updates
  // =========================================================================
  useEffect(() => {
    // Satellites and Labels
    entitiesRef.current.satellites.forEach((e) => {
      e.show = activeLayers.showSatellites;
      if (e.label) {
        e.label.show = new Cesium.ConstantProperty(activeLayers.showLabels);
      }
    });

    // Orbits
    entitiesRef.current.orbits.forEach((e) => {
      e.show = activeLayers.showOrbits;
    });

    // ISL Links
    entitiesRef.current.links.forEach((e) => {
      e.show = activeLayers.showIsl;
    });

    // Feeders
    entitiesRef.current.feeders.forEach((e) => {
      e.show = activeLayers.showFeeders;
    });

    // Access
    entitiesRef.current.access.forEach((e) => {
      e.show = activeLayers.showAccess;
    });

    // Gateways and Users
    entitiesRef.current.gateways.forEach((e) => {
      e.show = activeLayers.showGroundStations;
    });
    entitiesRef.current.users.forEach((e) => {
      e.show = activeLayers.showGroundStations;
    });
  }, [activeLayers]);

  // =========================================================================
  // Update Satellites Layer
  // =========================================================================
  useEffect(() => {
    const viewer = viewerRef.current;
    if (!viewer) return;

    const satEntityMap = entitiesRef.current.satellites;
    const currentSatIds = new Set<string>();

    for (const sat of satellites) {
      currentSatIds.add(sat.id);
      const isSelected = selectedEntityId === sat.id;

      let pos: Cesium.Cartesian3;
      if (sat.ecef && (sat.ecef.x !== 0 || sat.ecef.y !== 0 || sat.ecef.z !== 0)) {
        pos = ecefToCartesian(sat.ecef);
      } else {
        pos = geodeticToCartesian(sat.lat, sat.lon, sat.altKm || sat.altitudeKm || 600.0);
      }

      let entity = satEntityMap.get(sat.id);
      if (!entity) {
        entity = viewer.entities.add({
          id: `sat_${sat.id}`,
          name: sat.id,
          show: activeLayers.showSatellites,
          position: pos,
          point: {
            pixelSize: isSelected ? 11 : 7,
            color: isSelected
              ? Cesium.Color.YELLOW
              : Cesium.Color.fromCssColorString('#38bdf8'),
            outlineColor: Cesium.Color.WHITE,
            outlineWidth: isSelected ? 2 : 1,
          },
          label: {
            text: sat.id,
            show: activeLayers.showLabels,
            font: '10px monospace',
            fillColor: Cesium.Color.WHITE,
            showBackground: true,
            backgroundColor: Cesium.Color.fromCssColorString('rgba(15, 23, 42, 0.8)'),
            pixelOffset: new Cesium.Cartesian2(0, -14),
            distanceDisplayCondition: new Cesium.DistanceDisplayCondition(0, 2.5e7),
          },
        });
        satEntityMap.set(sat.id, entity);
      } else {
        // Smoothly update position
        entity.position = new Cesium.ConstantPositionProperty(pos);
        entity.show = activeLayers.showSatellites;
        if (entity.label) {
          entity.label.show = new Cesium.ConstantProperty(activeLayers.showLabels);
        }
        if (entity.point) {
          entity.point.pixelSize = new Cesium.ConstantProperty(isSelected ? 11 : 7);
          entity.point.color = new Cesium.ConstantProperty(
            isSelected ? Cesium.Color.YELLOW : Cesium.Color.fromCssColorString('#38bdf8')
          );
        }
      }
    }

    // Cleanup stale satellites
    for (const [id, entity] of satEntityMap.entries()) {
      if (!currentSatIds.has(id)) {
        viewer.entities.remove(entity);
        satEntityMap.delete(id);
      }
    }
  }, [satellites, selectedEntityId, activeLayers.showSatellites, activeLayers.showLabels]);

  // =========================================================================
  // Update Topology ISL Links Layer
  // =========================================================================
  useEffect(() => {
    const viewer = viewerRef.current;
    if (!viewer) return;

    const satMap = new Map<string, Satellite>();
    satellites.forEach((s) => satMap.set(s.id, s));

    const linkEntityMap = entitiesRef.current.links;
    const currentLinkKeys = new Set<string>();

    for (const link of topology) {
      const srcSat = satMap.get(link.src);
      const dstSat = satMap.get(link.dst);
      if (!srcSat || !dstSat) continue;

      const linkKey = [link.src, link.dst].sort().join('---');
      currentLinkKeys.add(linkKey);

      let p1: Cesium.Cartesian3;
      if (srcSat.ecef && (srcSat.ecef.x !== 0 || srcSat.ecef.y !== 0)) {
        p1 = ecefToCartesian(srcSat.ecef);
      } else {
        p1 = geodeticToCartesian(srcSat.lat, srcSat.lon, srcSat.altKm || srcSat.altitudeKm || 600.0);
      }

      let p2: Cesium.Cartesian3;
      if (dstSat.ecef && (dstSat.ecef.x !== 0 || dstSat.ecef.y !== 0)) {
        p2 = ecefToCartesian(dstSat.ecef);
      } else {
        p2 = geodeticToCartesian(dstSat.lat, dstSat.lon, dstSat.altKm || dstSat.altitudeKm || 600.0);
      }

      const isFailed = !link.active || link.status === 'FAILED';
      const isSelected = selectedEntityId === link.src || selectedEntityId === link.dst;

      let entity = linkEntityMap.get(linkKey);
      if (!entity) {
        entity = viewer.entities.add({
          id: `link_${linkKey}`,
          name: `${link.src} <-> ${link.dst}`,
          show: activeLayers.showIsl,
          polyline: {
            positions: [p1, p2],
            width: isFailed ? 3 : isSelected ? 2.5 : 1.5,
            material: isFailed ? createFailedLinkMaterial() : createHealthyIslMaterial(),
            arcType: Cesium.ArcType.NONE, // Straight laser beam
          },
        });
        linkEntityMap.set(linkKey, entity);
      } else {
        if (entity.polyline) {
          entity.polyline.positions = new Cesium.ConstantProperty([p1, p2]);
          entity.polyline.width = new Cesium.ConstantProperty(
            isFailed ? 3 : isSelected ? 2.5 : 1.5
          );
          entity.polyline.material = isFailed
            ? createFailedLinkMaterial()
            : createHealthyIslMaterial();
        }
      }
    }

    // Cleanup stale links
    for (const [key, entity] of linkEntityMap.entries()) {
      if (!currentLinkKeys.has(key)) {
        viewer.entities.remove(entity);
        linkEntityMap.delete(key);
      }
    }
  }, [topology, satellites, selectedEntityId, activeLayers.showIsl]);

  // =========================================================================
  // Update Ground Gateways & Feeder Beams Layer
  // =========================================================================
  useEffect(() => {
    const viewer = viewerRef.current;
    if (!viewer) return;

    const satMap = new Map<string, Satellite>();
    satellites.forEach((s) => satMap.set(s.id, s));

    const gwMap = entitiesRef.current.gateways;
    const feederMap = entitiesRef.current.feeders;
    const currentGwIds = new Set<string>();

    for (const gw of gateways) {
      currentGwIds.add(gw.id);
      const isSelected = selectedEntityId === gw.id;
      const gwPos = geodeticToCartesian(gw.lat, gw.lon, gw.altM || 50.0);

      // Gateway Pin
      let entity = gwMap.get(gw.id);
      if (!entity) {
        entity = viewer.entities.add({
          id: `gw_${gw.id}`,
          name: gw.id,
          show: activeLayers.showGroundStations,
          position: gwPos,
          point: {
            pixelSize: isSelected ? 12 : 9,
            color: Cesium.Color.fromCssColorString('#f59e0b'), // Amber
            outlineColor: Cesium.Color.WHITE,
            outlineWidth: 2,
          },
          label: {
            text: `GW: ${gw.id}`,
            show: activeLayers.showLabels,
            font: '10px monospace',
            fillColor: Cesium.Color.fromCssColorString('#fbbf24'),
            showBackground: true,
            backgroundColor: Cesium.Color.fromCssColorString('rgba(15, 23, 42, 0.85)'),
            pixelOffset: new Cesium.Cartesian2(0, -14),
            distanceDisplayCondition: new Cesium.DistanceDisplayCondition(0, 1.5e7),
          },
        });
        gwMap.set(gw.id, entity);
      } else {
        entity.position = new Cesium.ConstantPositionProperty(gwPos);
        if (entity.label) {
          entity.label.show = new Cesium.ConstantProperty(activeLayers.showLabels);
        }
        if (entity.point) {
          entity.point.pixelSize = new Cesium.ConstantProperty(isSelected ? 12 : 9);
        }
      }

      // Feeder Link to Connected Satellite
      if (gw.connectedSat && satMap.has(gw.connectedSat)) {
        const sat = satMap.get(gw.connectedSat)!;
        let satPos: Cesium.Cartesian3;
        if (sat.ecef && (sat.ecef.x !== 0 || sat.ecef.y !== 0)) {
          satPos = ecefToCartesian(sat.ecef);
        } else {
          satPos = geodeticToCartesian(sat.lat, sat.lon, sat.altKm || sat.altitudeKm || 600.0);
        }

        let feeder = feederMap.get(gw.id);
        if (!feeder) {
          feeder = viewer.entities.add({
            id: `feeder_${gw.id}`,
            name: `Feeder: ${gw.id} -> ${gw.connectedSat}`,
            show: activeLayers.showFeeders,
            polyline: {
              positions: [gwPos, satPos],
              width: 2.0,
              material: createFeederLinkMaterial(),
              arcType: Cesium.ArcType.NONE,
            },
          });
          feederMap.set(gw.id, feeder);
        } else {
          if (feeder.polyline) {
            feeder.polyline.positions = new Cesium.ConstantProperty([gwPos, satPos]);
          }
        }
      } else {
        const feeder = feederMap.get(gw.id);
        if (feeder) {
          viewer.entities.remove(feeder);
          feederMap.delete(gw.id);
        }
      }
    }

    // Cleanup stale gateways
    for (const [id, entity] of gwMap.entries()) {
      if (!currentGwIds.has(id)) {
        viewer.entities.remove(entity);
        gwMap.delete(id);
        const feeder = feederMap.get(id);
        if (feeder) {
          viewer.entities.remove(feeder);
          feederMap.delete(id);
        }
      }
    }
  }, [gateways, satellites, selectedEntityId, activeLayers.showGroundStations, activeLayers.showFeeders, activeLayers.showLabels]);

  // =========================================================================
  // Update Ground User Terminals & Access Beams Layer
  // =========================================================================
  useEffect(() => {
    const viewer = viewerRef.current;
    if (!viewer) return;

    const satMap = new Map<string, Satellite>();
    satellites.forEach((s) => satMap.set(s.id, s));

    const userMap = entitiesRef.current.users;
    const accessMap = entitiesRef.current.access;
    const currentUEIds = new Set<string>();

    for (const ue of users) {
      currentUEIds.add(ue.id);
      const isSelected = selectedEntityId === ue.id;
      const uePos = geodeticToCartesian(ue.lat, ue.lon, ue.altM || 10.0);

      // UE Dot
      let entity = userMap.get(ue.id);
      if (!entity) {
        entity = viewer.entities.add({
          id: `ue_${ue.id}`,
          name: ue.id,
          show: activeLayers.showGroundStations,
          position: uePos,
          point: {
            pixelSize: isSelected ? 9 : 6,
            color: Cesium.Color.fromCssColorString('#a855f7'), // Purple
            outlineColor: Cesium.Color.WHITE,
            outlineWidth: 1,
          },
          label: {
            text: ue.id,
            show: activeLayers.showLabels,
            font: '9px monospace',
            fillColor: Cesium.Color.fromCssColorString('#c084fc'),
            showBackground: true,
            backgroundColor: Cesium.Color.fromCssColorString('rgba(15, 23, 42, 0.8)'),
            pixelOffset: new Cesium.Cartesian2(0, -12),
            distanceDisplayCondition: new Cesium.DistanceDisplayCondition(0, 1.0e7),
          },
        });
        userMap.set(ue.id, entity);
      } else {
        entity.position = new Cesium.ConstantPositionProperty(uePos);
        if (entity.label) {
          entity.label.show = new Cesium.ConstantProperty(activeLayers.showLabels);
        }
        if (entity.point) {
          entity.point.pixelSize = new Cesium.ConstantProperty(isSelected ? 9 : 6);
        }
      }

      // 5G NR Access Link to Serving Satellite
      if (ue.servingSat && satMap.has(ue.servingSat)) {
        const sat = satMap.get(ue.servingSat)!;
        let satPos: Cesium.Cartesian3;
        if (sat.ecef && (sat.ecef.x !== 0 || sat.ecef.y !== 0)) {
          satPos = ecefToCartesian(sat.ecef);
        } else {
          satPos = geodeticToCartesian(sat.lat, sat.lon, sat.altKm || sat.altitudeKm || 600.0);
        }

        let access = accessMap.get(ue.id);
        if (!access) {
          access = viewer.entities.add({
            id: `access_${ue.id}`,
            name: `Access: ${ue.id} -> ${ue.servingSat}`,
            show: activeLayers.showAccess,
            polyline: {
              positions: [uePos, satPos],
              width: 1.2,
              material: createAccessLinkMaterial(),
              arcType: Cesium.ArcType.NONE,
            },
          });
          accessMap.set(ue.id, access);
        } else {
          if (access.polyline) {
            access.polyline.positions = new Cesium.ConstantProperty([uePos, satPos]);
          }
        }
      } else {
        const access = accessMap.get(ue.id);
        if (access) {
          viewer.entities.remove(access);
          accessMap.delete(ue.id);
        }
      }
    }

    // Cleanup stale users
    for (const [id, entity] of userMap.entries()) {
      if (!currentUEIds.has(id)) {
        viewer.entities.remove(entity);
        userMap.delete(id);
        const access = accessMap.get(id);
        if (access) {
          viewer.entities.remove(access);
          accessMap.delete(id);
        }
      }
    }
  }, [users, satellites, selectedEntityId, activeLayers.showGroundStations, activeLayers.showAccess, activeLayers.showLabels]);

  // =========================================================================
  // Camera Controls
  // =========================================================================
  const handleResetCamera = useCallback(() => {
    if (!viewerRef.current) return;
    viewerRef.current.camera.flyTo({
      destination: Cesium.Cartesian3.fromDegrees(10.0, 30.0, 22000000.0),
      duration: 1.2,
    });
  }, []);

  const handleZoomIn = useCallback(() => {
    if (!viewerRef.current) return;
    viewerRef.current.camera.zoomIn(1500000);
  }, []);

  const handleZoomOut = useCallback(() => {
    if (!viewerRef.current) return;
    viewerRef.current.camera.zoomOut(1500000);
  }, []);

  return (
    <div className="relative w-full h-full overflow-hidden bg-slate-950">
      <div ref={containerRef} className="w-full h-full" />
      <GlobeControls
        layers={activeLayers}
        onToggleLayer={toggleLayer}
        onResetCamera={handleResetCamera}
        onZoomIn={handleZoomIn}
        onZoomOut={handleZoomOut}
      />
    </div>
  );
};
