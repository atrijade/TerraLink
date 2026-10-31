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
}

export const GlobeViewer: React.FC<GlobeViewerProps> = ({
  satellites,
  topology,
  gateways,
  users,
  selectedEntityId,
  onSelectEntity,
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

  const [layers, setLayers] = useState<GlobeLayerState>({
    showOrbits: true,
    showIsl: true,
    showFeeders: true,
    showAccess: true,
    showGroundStations: true,
  });

  const toggleLayer = (key: keyof GlobeLayerState) => {
    setLayers((prev) => ({ ...prev, [key]: !prev[key] }));
  };

  // =========================================================================
  // Initialize Cesium Viewer
  // =========================================================================
  useEffect(() => {
    if (!containerRef.current) return;

    // Use dark CartoDB basemap imagery with OSM fallback
    let baseLayer: Cesium.ImageryLayer | false = false;
    try {
      baseLayer = new Cesium.ImageryLayer(
        new Cesium.UrlTemplateImageryProvider({
          url: 'https://basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png',
          credit: 'CartoDB / OpenStreetMap',
        })
      );
    } catch {
      baseLayer = false;
    }

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
      shouldAnimate: true,
      baseLayer: baseLayer,
    });

    viewerRef.current = viewer;

    // Configure scene styling for aerospace HUD
    const scene = viewer.scene;
    scene.globe.baseColor = Cesium.Color.fromCssColorString('#020617');
    scene.globe.enableLighting = false;
    scene.globe.showGroundAtmosphere = true;
    scene.backgroundColor = Cesium.Color.fromCssColorString('#020617');

    if (scene.skyAtmosphere) {
      scene.skyAtmosphere.show = true;
    }

    // Set initial orbital camera vantage point
    viewer.camera.setView({
      destination: Cesium.Cartesian3.fromDegrees(10.0, 30.0, 22000000.0),
      orientation: {
        heading: Cesium.Math.toRadians(0),
        pitch: Cesium.Math.toRadians(-85),
        roll: 0.0,
      },
    });

    // Handle Click on 3D Entities
    const handler = new Cesium.ScreenSpaceEventHandler(scene.canvas);
    handler.setInputAction((click: Cesium.ScreenSpaceEventHandler.PositionedEvent) => {
      const pickedObject = scene.pick(click.position);
      if (Cesium.defined(pickedObject) && pickedObject.id && pickedObject.id.name) {
        onSelectEntity(pickedObject.id.name);
      } else {
        onSelectEntity(null);
      }
    }, Cesium.ScreenSpaceEventType.LEFT_CLICK);

    // Orbital Plane Rings (draw 4 Walker orbital plane ribbons)
    const orbitEntities: Cesium.Entity[] = [];
    for (let p = 0; p < 4; p++) {
      const raan = (p * 360.0) / 4.0;
      const positions = generateOrbitalTrackPositions(53.0, raan, 600.0, 90);

      const orbitRing = viewer.entities.add({
        name: `Orbit_Plane_${p}`,
        polyline: {
          positions: positions,
          width: 1.5,
          material: new Cesium.PolylineGlowMaterialProperty({
            glowPower: 0.1,
            color: Cesium.Color.fromCssColorString('rgba(56, 189, 248, 0.25)'),
          }),
        },
      });
      orbitEntities.push(orbitRing);
    }
    entitiesRef.current.orbits = orbitEntities;

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
    // Orbits
    entitiesRef.current.orbits.forEach((e) => {
      e.show = layers.showOrbits;
    });

    // ISL Links
    entitiesRef.current.links.forEach((e) => {
      e.show = layers.showIsl;
    });

    // Feeders
    entitiesRef.current.feeders.forEach((e) => {
      e.show = layers.showFeeders;
    });

    // Access
    entitiesRef.current.access.forEach((e) => {
      e.show = layers.showAccess;
    });

    // Gateways and Users
    entitiesRef.current.gateways.forEach((e) => {
      e.show = layers.showGroundStations;
    });
    entitiesRef.current.users.forEach((e) => {
      e.show = layers.showGroundStations;
    });
  }, [layers]);

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
  }, [satellites, selectedEntityId]);

  // =========================================================================
  // Update Topology ISL Links Layer
  // =========================================================================
  useEffect(() => {
    const viewer = viewerRef.current;
    if (!viewer) return;

    const linkEntityMap = entitiesRef.current.links;
    const currentLinkIds = new Set<string>();

    const satPosMap = new Map<string, Cesium.Cartesian3>();
    for (const sat of satellites) {
      if (sat.ecef && (sat.ecef.x !== 0 || sat.ecef.y !== 0 || sat.ecef.z !== 0)) {
        satPosMap.set(sat.id, ecefToCartesian(sat.ecef));
      } else {
        satPosMap.set(
          sat.id,
          geodeticToCartesian(sat.lat, sat.lon, sat.altKm || sat.altitudeKm || 600.0)
        );
      }
    }

    for (const link of topology) {
      const srcPos = satPosMap.get(link.src);
      const dstPos = satPosMap.get(link.dst);
      if (!srcPos || !dstPos) continue;

      const linkKey = [link.src, link.dst].sort().join('---');
      currentLinkIds.add(linkKey);

      const isFailed = !link.active || link.status === 'FAILED';
      const isSelected = selectedEntityId === link.src || selectedEntityId === link.dst;

      let entity = linkEntityMap.get(linkKey);
      if (!entity) {
        entity = viewer.entities.add({
          id: `isl_${linkKey}`,
          name: `ISL: ${link.src} <-> ${link.dst}`,
          polyline: {
            positions: [srcPos, dstPos],
            width: isFailed ? 2.5 : isSelected ? 2.5 : 1.5,
            material: isFailed ? createFailedLinkMaterial() : createHealthyIslMaterial(),
          },
          show: layers.showIsl,
        });
        linkEntityMap.set(linkKey, entity);
      } else {
        if (entity.polyline) {
          entity.polyline.positions = new Cesium.ConstantProperty([srcPos, dstPos]);
          entity.polyline.material = isFailed
            ? createFailedLinkMaterial()
            : createHealthyIslMaterial();
          entity.polyline.width = new Cesium.ConstantProperty(
            isFailed ? 2.5 : isSelected ? 2.5 : 1.5
          );
        }
      }
    }

    // Cleanup stale links
    for (const [key, entity] of linkEntityMap.entries()) {
      if (!currentLinkIds.has(key)) {
        viewer.entities.remove(entity);
        linkEntityMap.delete(key);
      }
    }
  }, [topology, satellites, selectedEntityId, layers.showIsl]);

  // =========================================================================
  // Update Gateways & Feeder Links Layer
  // =========================================================================
  useEffect(() => {
    const viewer = viewerRef.current;
    if (!viewer) return;

    const gwEntityMap = entitiesRef.current.gateways;
    const feederEntityMap = entitiesRef.current.feeders;
    const currentGwIds = new Set<string>();

    const satPosMap = new Map<string, Cesium.Cartesian3>();
    for (const sat of satellites) {
      if (sat.ecef && (sat.ecef.x !== 0 || sat.ecef.y !== 0 || sat.ecef.z !== 0)) {
        satPosMap.set(sat.id, ecefToCartesian(sat.ecef));
      } else {
        satPosMap.set(sat.id, geodeticToCartesian(sat.lat, sat.lon, sat.altKm || 600.0));
      }
    }

    for (const gw of gateways) {
      currentGwIds.add(gw.id);
      const gwPos = geodeticToCartesian(gw.lat, gw.lon, gw.altM / 1000.0);

      // 1. Gateway marker
      let gwEntity = gwEntityMap.get(gw.id);
      if (!gwEntity) {
        gwEntity = viewer.entities.add({
          id: `gw_${gw.id}`,
          name: gw.id,
          position: gwPos,
          point: {
            pixelSize: 9,
            color: Cesium.Color.fromCssColorString('#10b981'), // Emerald
            outlineColor: Cesium.Color.WHITE,
            outlineWidth: 1.5,
          },
          label: {
            text: `🛰️ ${gw.id}`,
            font: '10px sans-serif',
            fillColor: Cesium.Color.fromCssColorString('#6ee7b7'),
            showBackground: true,
            backgroundColor: Cesium.Color.fromCssColorString('rgba(6, 78, 59, 0.8)'),
            pixelOffset: new Cesium.Cartesian2(0, 14),
            distanceDisplayCondition: new Cesium.DistanceDisplayCondition(0, 1.8e7),
          },
          show: layers.showGroundStations,
        });
        gwEntityMap.set(gw.id, gwEntity);
      } else {
        gwEntity.position = new Cesium.ConstantPositionProperty(gwPos);
      }

      // 2. Feeder Link to Connected Satellite
      if (gw.connectedSat && satPosMap.has(gw.connectedSat)) {
        const satPos = satPosMap.get(gw.connectedSat)!;
        let feederEntity = feederEntityMap.get(gw.id);

        if (!feederEntity) {
          feederEntity = viewer.entities.add({
            id: `feeder_${gw.id}`,
            name: `Feeder: ${gw.id} -> ${gw.connectedSat}`,
            polyline: {
              positions: [gwPos, satPos],
              width: 2.0,
              material: createFeederLinkMaterial(),
            },
            show: layers.showFeeders,
          });
          feederEntityMap.set(gw.id, feederEntity);
        } else {
          if (feederEntity.polyline) {
            feederEntity.polyline.positions = new Cesium.ConstantProperty([gwPos, satPos]);
          }
        }
      }
    }

    // Cleanup stale gateways
    for (const [id, entity] of gwEntityMap.entries()) {
      if (!currentGwIds.has(id)) {
        viewer.entities.remove(entity);
        gwEntityMap.delete(id);
      }
    }
  }, [gateways, satellites, layers.showGroundStations, layers.showFeeders]);

  // =========================================================================
  // Update Users & Access Links Layer
  // =========================================================================
  useEffect(() => {
    const viewer = viewerRef.current;
    if (!viewer) return;

    const userEntityMap = entitiesRef.current.users;
    const accessEntityMap = entitiesRef.current.access;
    const currentUserIds = new Set<string>();

    const satPosMap = new Map<string, Cesium.Cartesian3>();
    for (const sat of satellites) {
      if (sat.ecef && (sat.ecef.x !== 0 || sat.ecef.y !== 0 || sat.ecef.z !== 0)) {
        satPosMap.set(sat.id, ecefToCartesian(sat.ecef));
      } else {
        satPosMap.set(sat.id, geodeticToCartesian(sat.lat, sat.lon, sat.altKm || 600.0));
      }
    }

    for (const u of users) {
      currentUserIds.add(u.id);
      const userPos = geodeticToCartesian(u.lat, u.lon, 0);

      // 1. User marker
      let uEntity = userEntityMap.get(u.id);
      if (!uEntity) {
        uEntity = viewer.entities.add({
          id: `user_${u.id}`,
          name: u.id,
          position: userPos,
          point: {
            pixelSize: 6,
            color: Cesium.Color.fromCssColorString('#a855f7'), // Purple/Violet
            outlineColor: Cesium.Color.WHITE,
            outlineWidth: 1,
          },
          label: {
            text: u.id,
            font: '9px sans-serif',
            fillColor: Cesium.Color.WHITE,
            pixelOffset: new Cesium.Cartesian2(0, 10),
            distanceDisplayCondition: new Cesium.DistanceDisplayCondition(0, 1.2e7),
          },
          show: layers.showGroundStations,
        });
        userEntityMap.set(u.id, uEntity);
      } else {
        uEntity.position = new Cesium.ConstantPositionProperty(userPos);
      }

      // 2. Access Link to Serving Satellite
      if (u.servingSat && satPosMap.has(u.servingSat)) {
        const satPos = satPosMap.get(u.servingSat)!;
        let accessEntity = accessEntityMap.get(u.id);

        if (!accessEntity) {
          accessEntity = viewer.entities.add({
            id: `access_${u.id}`,
            name: `Access: ${u.id} -> ${u.servingSat}`,
            polyline: {
              positions: [userPos, satPos],
              width: 1.5,
              material: createAccessLinkMaterial(),
            },
            show: layers.showAccess,
          });
          accessEntityMap.set(u.id, accessEntity);
        } else {
          if (accessEntity.polyline) {
            accessEntity.polyline.positions = new Cesium.ConstantProperty([userPos, satPos]);
          }
        }
      }
    }

    // Cleanup stale users
    for (const [id, entity] of userEntityMap.entries()) {
      if (!currentUserIds.has(id)) {
        viewer.entities.remove(entity);
        userEntityMap.delete(id);
      }
    }
  }, [users, satellites, layers.showGroundStations, layers.showAccess]);

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
        layers={layers}
        onToggleLayer={toggleLayer}
        onResetCamera={handleResetCamera}
        onZoomIn={handleZoomIn}
        onZoomOut={handleZoomOut}
      />
    </div>
  );
};
