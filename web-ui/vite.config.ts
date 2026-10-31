import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import cesium from 'vite-plugin-cesium';
import tailwindcss from '@tailwindcss/vite';

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    react(),
    cesium(),
    tailwindcss(),
  ],
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
      '/satellites': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
      '/topology': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
      '/gateways': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
      '/users': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
      '/metrics': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
      '/status': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
      '/start': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
      '/stop': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
    },
  },
});
