declare global { interface Window { __AAJA_TEST_MOCK__?: boolean; } }
if (import.meta.env.VITE_AAJA_MOCK === "1") window.__AAJA_TEST_MOCK__ = true;
import "./ui/styles.css";
import { GameBootstrap } from "./bootstrap";

const app = document.querySelector<HTMLElement>("#app");
if (!app) throw new Error("Missing #app root");
const bootstrap = new GameBootstrap(app);
bootstrap.attach();
