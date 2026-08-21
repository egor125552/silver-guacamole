export class PluginHost {
  constructor(game) {
    this.game = game;
    this.loaded = new Map();
  }
  async load(definitions) {
    for (const definition of definitions) {
      if (definition.enabled === false) continue;
      const module = await import(definition.module);
      const plugin = module.default;
      if (!plugin?.id || typeof plugin.install !== "function") {
        throw new Error(`Invalid plugin: ${definition.module}`);
      }
      const cleanup = await plugin.install(this.game);
      this.loaded.set(plugin.id, typeof cleanup === "function" ? cleanup : null);
    }
  }
  unload(id) {
    this.loaded.get(id)?.();
    this.loaded.delete(id);
  }
}
