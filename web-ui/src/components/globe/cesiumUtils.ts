import * as Cesium from 'cesium';
import type { Satellite } from '../../api/types';

/**
 * Converts Geodetic coordinates (lat, lon, altKm) to Cesium Cartesian3.
 */
export function geodeticToCartesian(lat: number, lon: number, altKm: number = 0): Cesium.Cartesian3 {
  return Cesium.Cartesian3.fromDegrees(lon, lat, altKm * 1000.0);
}

/**
 * Converts ECEF meters to Cesium Cartesian3.
 */
export function ecefToCartesian(ecef: { x: number; y: number; z: number }): Cesium.Cartesian3 {
  return new Cesium.Cartesian3(ecef.x, ecef.y, ecef.z);
}

/**
 * Computes an orbital trajectory circle for a given plane.
 */
export function generateOrbitalTrackPositions(
  inclinationDeg: number = 53.0,
  raanDeg: number = 0,
  altitudeKm: number = 600.0,
  segments: number = 90
): Cesium.Cartesian3[] {
  const points: Cesium.Cartesian3[] = [];
  const incRad = (inclinationDeg * Math.PI) / 180.0;
  const raanRad = (raanDeg * Math.PI) / 180.0;
  const rMeters = (6371.0 + altitudeKm) * 1000.0;

  for (let i = 0; i <= segments; i++) {
    const u = (i / segments) * 2 * Math.PI;

    // Orbital plane coordinates
    const xOrb = rMeters * Math.cos(u);
    const yOrb = rMeters * Math.sin(u);

    // Inertial coordinates
    const x = xOrb * Math.cos(raanRad) - yOrb * Math.cos(incRad) * Math.sin(raanRad);
    const y = xOrb * Math.sin(raanRad) + yOrb * Math.cos(incRad) * Math.cos(raanRad);
    const z = yOrb * Math.sin(incRad);

    points.push(new Cesium.Cartesian3(x, y, z));
  }

  return points;
}

/**
 * Creates a glowing cyan/green material for healthy ISL links.
 */
export function createHealthyIslMaterial() {
  return new Cesium.PolylineGlowMaterialProperty({
    glowPower: 0.25,
    color: Cesium.Color.fromCssColorString('#06b6d4'), // Cyan
  });
}

/**
 * Creates a red dashed pulsing material for severed links.
 */
export function createFailedLinkMaterial() {
  return new Cesium.PolylineDashMaterialProperty({
    color: Cesium.Color.RED,
    gapColor: Cesium.Color.TRANSPARENT,
    dashLength: 20.0,
  });
}

/**
 * Creates a golden feeder link material (Satellite <-> Gateway).
 */
export function createFeederLinkMaterial() {
  return new Cesium.PolylineGlowMaterialProperty({
    glowPower: 0.3,
    color: Cesium.Color.fromCssColorString('#eab308'), // Gold/Amber
  });
}

/**
 * Creates a blue dashed access link material (UE <-> Satellite).
 */
export function createAccessLinkMaterial() {
  return new Cesium.PolylineDashMaterialProperty({
    color: Cesium.Color.fromCssColorString('#60a5fa'), // Light Blue
    gapColor: Cesium.Color.TRANSPARENT,
    dashLength: 14.0,
  });
}

/**
 * Finds a satellite's Cartesian3 position from an array.
 */
export function getSatelliteCartesian(
  satellites: Satellite[],
  satId: string
): Cesium.Cartesian3 | null {
  const sat = satellites.find((s) => s.id === satId);
  if (!sat) return null;
  if (sat.ecef && (sat.ecef.x !== 0 || sat.ecef.y !== 0 || sat.ecef.z !== 0)) {
    return ecefToCartesian(sat.ecef);
  }
  return geodeticToCartesian(sat.lat, sat.lon, sat.altKm || sat.altitudeKm || 600);
}
