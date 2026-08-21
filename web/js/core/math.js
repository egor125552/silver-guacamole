export const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
export const rand = (lo, hi) => lo + Math.random() * (hi - lo);
export const randInt = (lo, hi) => Math.floor(rand(lo, hi + 1));
export const distance = (a, b) => Math.hypot(a.x - b.x, a.z - b.z);
export function normalized(v) {
  const len = Math.hypot(v.x, v.z);
  return len < 1e-4 ? {x:0,z:0} : {x:v.x/len,z:v.z/len};
}
export function rotate(v, a) {
  const c = Math.cos(a), s = Math.sin(a);
  return {x:v.x*c-v.z*s, z:v.x*s+v.z*c};
}
export function rectIntersects(a,b) {
  return a.x < b.x+b.w && a.x+a.w > b.x && a.z < b.z+b.h && a.z+a.h > b.z;
}
export const entityRect = (p, r=.4) => ({x:p.x-r,z:p.z-r,w:r*2,h:r*2});
export function pointInRect(p,r){ return p.x>=r.x && p.x<=r.x+r.w && p.z>=r.z && p.z<=r.z+r.h; }

export function rayRect(origin, direction, rect) {
  let tmin = -Infinity, tmax = Infinity;
  const axes = [["x", rect.x, rect.x+rect.w], ["z", rect.z, rect.z+rect.h]];
  for (const [axis,min,max] of axes) {
    const d = direction[axis], o = origin[axis];
    if (Math.abs(d) < 1e-8) {
      if (o < min || o > max) return null;
      continue;
    }
    let t1=(min-o)/d, t2=(max-o)/d;
    if (t1>t2) [t1,t2]=[t2,t1];
    tmin=Math.max(tmin,t1); tmax=Math.min(tmax,t2);
    if (tmin>tmax) return null;
  }
  const t = tmin >= 0 ? tmin : tmax >= 0 ? tmax : null;
  return t == null ? null : {x:origin.x+direction.x*t,z:origin.z+direction.z*t,t};
}
export function hasLineOfSight(from,to,walls) {
  const d = normalized({x:to.x-from.x,z:to.z-from.z});
  const max = distance(from,to);
  if (max < 1e-4) return true;
  return !walls.some(w => {
    const h=rayRect(from,d,w);
    return h && h.t < max-.001;
  });
}
export function wallsBetween(from,to,walls) {
  const d = normalized({x:to.x-from.x,z:to.z-from.z});
  const max = distance(from,to);
  if (max < 1e-4) return 0;
  return walls.reduce((n,w)=>{
    const h=rayRect(from,d,w);
    return n + (h && h.t < max-.05 ? 1 : 0);
  },0);
}
