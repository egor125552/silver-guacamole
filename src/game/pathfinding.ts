import type { GridCell, Point } from "./types";
import { cellCenter, cellKey, GRID_HEIGHT, GRID_WIDTH, worldToCell } from "../world/WorldMap";

export interface PathOptions {
  blocked: ReadonlySet<string>;
  width?: number;
  height?: number;
  allowGoalBlocked?: boolean;
}

const directions: readonly GridCell[] = [
  { col: 1, row: 0 },
  { col: 0, row: 1 },
  { col: -1, row: 0 },
  { col: 0, row: -1 },
];
const heuristic = (a: GridCell, b: GridCell): number => Math.abs(a.col - b.col) + Math.abs(a.row - b.row);
const parseKey = (value: string): GridCell => {
  const parts = value.split(",");
  return { col: Number(parts[0] ?? 0), row: Number(parts[1] ?? 0) };
};

export function nearestWalkableCell(cell: GridCell, blocked: ReadonlySet<string>, width = GRID_WIDTH, height = GRID_HEIGHT): GridCell | null {
  if (!blocked.has(cellKey(cell))) return cell;
  for (let radius = 1; radius < Math.max(width, height); radius += 1) {
    for (let row = cell.row - radius; row <= cell.row + radius; row += 1) {
      for (let col = cell.col - radius; col <= cell.col + radius; col += 1) {
        if (col < 0 || row < 0 || col >= width || row >= height) continue;
        if (Math.abs(col - cell.col) !== radius && Math.abs(row - cell.row) !== radius) continue;
        const candidate = { col, row };
        if (!blocked.has(cellKey(candidate))) return candidate;
      }
    }
  }
  return null;
}

export function findGridPath(start: GridCell, goal: GridCell, options: PathOptions): GridCell[] {
  const width = options.width ?? GRID_WIDTH;
  const height = options.height ?? GRID_HEIGHT;
  const realStart = nearestWalkableCell(start, options.blocked, width, height);
  const realGoal = options.allowGoalBlocked ? goal : nearestWalkableCell(goal, options.blocked, width, height);
  if (!realStart || !realGoal) return [];
  const startKey = cellKey(realStart);
  const goalKey = cellKey(realGoal);
  if (startKey === goalKey) return [realStart];

  const open = new Set<string>([startKey]);
  const cameFrom = new Map<string, string>();
  const g = new Map<string, number>([[startKey, 0]]);
  const f = new Map<string, number>([[startKey, heuristic(realStart, realGoal)]]);

  while (open.size > 0) {
    let currentKey = "";
    let currentScore = Number.POSITIVE_INFINITY;
    for (const candidate of open) {
      const score = f.get(candidate) ?? Number.POSITIVE_INFINITY;
      if (score < currentScore || (score === currentScore && candidate < currentKey)) { currentKey = candidate; currentScore = score; }
    }
    if (currentKey === goalKey) {
      const path: GridCell[] = [parseKey(currentKey)];
      while (cameFrom.has(currentKey)) { currentKey = cameFrom.get(currentKey)!; path.push(parseKey(currentKey)); }
      return path.reverse();
    }
    open.delete(currentKey);
    const current = parseKey(currentKey);
    for (const direction of directions) {
      const next = { col: current.col + direction.col, row: current.row + direction.row };
      if (next.col < 0 || next.row < 0 || next.col >= width || next.row >= height) continue;
      const nextKey = cellKey(next);
      if (options.blocked.has(nextKey) && nextKey !== goalKey) continue;
      const tentative = (g.get(currentKey) ?? Number.POSITIVE_INFINITY) + 1;
      if (tentative >= (g.get(nextKey) ?? Number.POSITIVE_INFINITY)) continue;
      cameFrom.set(nextKey, currentKey);
      g.set(nextKey, tentative);
      f.set(nextKey, tentative + heuristic(next, realGoal));
      open.add(nextKey);
    }
  }
  return [];
}

export function findWorldPath(start: Point, goal: Point, blocked: ReadonlySet<string>): Point[] {
  return findGridPath(worldToCell(start), worldToCell(goal), { blocked }).map((cell) => cellCenter(cell.col, cell.row));
}
