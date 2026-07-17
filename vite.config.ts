import { defineConfig } from "vite";

export default defineConfig(({ mode }) => ({
  base: mode === "development" ? "/" : "/silver-guacamole/",
  build: {
    target: "es2022",
    sourcemap: true,
    assetsInlineLimit: 0,
    chunkSizeWarningLimit: 1700,
    rollupOptions: {
      output: {
        manualChunks: (id: string) => id.includes("node_modules/phaser") ? "phaser" : undefined,
      },
    },
  },
  server: { host: "127.0.0.1", port: 5173 },
  preview: { host: "127.0.0.1", port: 4173 },
}));
