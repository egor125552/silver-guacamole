import {normalized,rotate,entityRect,rectIntersects} from "../../core/math.js";
const angles=[0,.42,-.42,.82,-.82,1.20,-1.20,1.57,-1.57];
function blocked(game,pos){const b=entityRect(pos,.4);return game.state.walls.some(w=>rectIntersects(b,w));}
export default{id:"enemy-navigation",install(game){game.moveEnemy=(e,direction,speed,dt)=>{const n=normalized(direction);if(Math.hypot(n.x,n.z)<1e-4)return false;const step=speed*dt;for(const a of angles){const d=rotate(n,a),p={x:e.position.x+d.x*step,z:e.position.z+d.z*step};if(!blocked(game,p)){e.position=p;return true;}}return false;};}};
