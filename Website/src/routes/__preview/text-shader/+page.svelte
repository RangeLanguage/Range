<script lang="ts">
  import { onMount } from "svelte";

  type ShadowSettings = {
    label: string;
    color: string;
    angle: number;
    distance: number;
    blur: number;
    spread: number;
    opacity: number;
  };

  let sample = $state("Range");
  let a4Canvas: HTMLCanvasElement;
  let a4Sheet: HTMLElement;
  let shaderAvailable = $state(true);
  let baseColor = $state("#c6cfe2");
  let outer90 = $state<ShadowSettings>({
    label: "Outer · 90°",
    color: "#111318",
    angle: 90,
    distance: 13,
    blur: 10,
    spread: 0,
    opacity: 70,
  });
  let outer270 = $state<ShadowSettings>({
    label: "Outer · 270°",
    color: "#17191e",
    angle: 270,
    distance: 10,
    blur: 15,
    spread: 0,
    opacity: 50,
  });
  let inner90 = $state<ShadowSettings>({
    label: "Inner · 90°",
    color: "#ffffff",
    angle: 90,
    distance: 15,
    blur: 10,
    spread: 1.5,
    opacity: 85,
  });
  let inner270 = $state<ShadowSettings>({
    label: "Inner · 270°",
    color: "#ffffff",
    angle: 270,
    distance: 15,
    blur: 9,
    spread: 5,
    opacity: 85,
  });

  const displayText = $derived(sample.trim() || "Range");

  function offset(angle: number, distance: number, inner = false) {
    const radians = (angle * Math.PI) / 180;
    const direction = inner ? -1 : 1;
    return {
      x: Math.cos(radians) * distance * direction,
      y: Math.sin(radians) * distance * direction,
    };
  }

  onMount(() => {
    const context = a4Canvas.getContext("webgl", {
      alpha: false,
      antialias: false,
      depth: false,
      powerPreference: "high-performance",
    });

    if (!context) {
      shaderAvailable = false;
      return;
    }
    const gl = context;

    const vertexSource = `
      attribute vec2 a_position;
      void main() {
        gl_Position = vec4(a_position, 0.0, 1.0);
      }
    `;

    const fragmentSource = `
      precision highp float;

      uniform vec2 u_resolution;
      uniform vec2 u_pointer;
      uniform float u_time;

      float hash(vec2 point) {
        return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453);
      }

      float noise(vec2 point) {
        vec2 cell = floor(point);
        vec2 local = fract(point);
        local = local * local * (3.0 - 2.0 * local);
        return mix(
          mix(hash(cell), hash(cell + vec2(1.0, 0.0)), local.x),
          mix(hash(cell + vec2(0.0, 1.0)), hash(cell + vec2(1.0)), local.x),
          local.y
        );
      }

      float fbm(vec2 point) {
        float value = 0.0;
        float amplitude = 0.52;
        mat2 rotation = mat2(0.82, -0.57, 0.57, 0.82);
        for (int octave = 0; octave < 5; octave += 1) {
          value += amplitude * noise(point);
          point = rotation * point * 2.03 + 7.31;
          amplitude *= 0.5;
        }
        return value;
      }

      float radialSpot(vec2 point, vec2 center, float radius) {
        float normalizedDistance = length(point - center) / radius;
        return 1.0 - smoothstep(0.0, 1.0, normalizedDistance);
      }

      void main() {
        vec2 resolution = max(u_resolution, vec2(1.0));
        vec2 uv = gl_FragCoord.xy / resolution;
        vec2 point = uv - 0.5;
        point.x *= resolution.x / resolution.y;

        float drift = sin(u_time * 0.24) * 0.018;
        vec2 amberCenter = vec2(-0.30, 0.38 + drift) + u_pointer * vec2(0.025, 0.018);
        vec2 blueCenter = vec2(0.25, 0.02 - drift) - u_pointer * vec2(0.02, 0.024);

        float amberField = radialSpot(point, amberCenter, 0.78);
        float blueField = radialSpot(point, blueCenter, 0.56);
        float lowerDistortion = (fbm(vec2(point.x * 2.7, point.y * 1.35) + u_time * 0.025) - 0.5) * 0.13;
        lowerDistortion += sin(point.x * 10.0 + u_time * 0.18) * 0.018;
        float lowerRaw = 1.0 - smoothstep(0.08, 0.62, uv.y + lowerDistortion);
        float lowerField = clamp(lowerRaw, 0.0, 1.0);

        vec3 paper = vec3(0.985, 0.975, 0.95);
        vec3 amberEdge = vec3(1.0, 0.78, 0.045);
        vec3 amberCore = vec3(1.0, 0.235, 0.0);
        vec3 blueEdge = vec3(0.08, 0.78, 1.0);
        vec3 blueCore = vec3(0.0, 0.105, 1.0);
        vec3 amber = mix(amberEdge, amberCore, amberField);
        vec3 blue = mix(blueEdge, blueCore, blueField);

        vec3 color = mix(paper, amber, amberField * 0.96);
        color = mix(color, blue, blueField * 0.9);

        float overlap = min(amberField, blueField);
        float burnWeight = 1.0 - exp(-7.5 * overlap * overlap);
        vec3 burnEdge = vec3(0.88, 0.22, 0.012);
        vec3 burnCore = vec3(0.24, 0.035, 0.006);
        vec3 burntOrange = mix(burnEdge, burnCore, smoothstep(0.22, 0.92, overlap));
        color = mix(color, burntOrange, burnWeight * 0.94);
        color = mix(color, vec3(1.0), lowerField);

        color = pow(max(color, 0.0), vec3(0.94));
        gl_FragColor = vec4(color, 1.0);
      }
    `;

    function compile(type: number, source: string) {
      const shader = gl.createShader(type);
      if (!shader) throw new Error("Unable to create A4 shader.");
      gl.shaderSource(shader, source);
      gl.compileShader(shader);
      if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        throw new Error(gl.getShaderInfoLog(shader) ?? "A4 shader compilation failed.");
      }
      return shader;
    }

    let program: WebGLProgram;
    try {
      const vertex = compile(gl.VERTEX_SHADER, vertexSource);
      const fragment = compile(gl.FRAGMENT_SHADER, fragmentSource);
      const createdProgram = gl.createProgram();
      if (!createdProgram) throw new Error("Unable to create A4 shader program.");
      gl.attachShader(createdProgram, vertex);
      gl.attachShader(createdProgram, fragment);
      gl.linkProgram(createdProgram);
      if (!gl.getProgramParameter(createdProgram, gl.LINK_STATUS)) {
        throw new Error(gl.getProgramInfoLog(createdProgram) ?? "A4 shader link failed.");
      }
      program = createdProgram;
      gl.deleteShader(vertex);
      gl.deleteShader(fragment);
    } catch (error) {
      console.error(error);
      shaderAvailable = false;
      return;
    }

    const position = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, position);
    gl.bufferData(
      gl.ARRAY_BUFFER,
      new Float32Array([-1, -1, 1, -1, -1, 1, -1, 1, 1, -1, 1, 1]),
      gl.STATIC_DRAW,
    );

    const positionLocation = gl.getAttribLocation(program, "a_position");
    const resolutionLocation = gl.getUniformLocation(program, "u_resolution");
    const pointerLocation = gl.getUniformLocation(program, "u_pointer");
    const timeLocation = gl.getUniformLocation(program, "u_time");
    const startedAt = performance.now();
    const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    let pointerX = 0;
    let pointerY = 0;
    let targetX = 0;
    let targetY = 0;
    let frame = 0;

    function handlePointer(event: PointerEvent) {
      const bounds = a4Sheet.getBoundingClientRect();
      targetX = ((event.clientX - bounds.left) / bounds.width) * 2 - 1;
      targetY = -(((event.clientY - bounds.top) / bounds.height) * 2 - 1);
    }

    function render(now: number) {
      pointerX += (targetX - pointerX) * 0.06;
      pointerY += (targetY - pointerY) * 0.06;
      const density = Math.min(window.devicePixelRatio || 1, 1.5);
      const width = Math.max(1, Math.round(a4Canvas.clientWidth * density));
      const height = Math.max(1, Math.round(a4Canvas.clientHeight * density));
      if (a4Canvas.width !== width || a4Canvas.height !== height) {
        a4Canvas.width = width;
        a4Canvas.height = height;
      }
      gl.viewport(0, 0, width, height);
      gl.useProgram(program);
      gl.enableVertexAttribArray(positionLocation);
      gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);
      gl.uniform2f(resolutionLocation, width, height);
      gl.uniform2f(pointerLocation, pointerX, pointerY);
      gl.uniform1f(timeLocation, reducedMotion ? 0 : (now - startedAt) / 1000);
      gl.drawArrays(gl.TRIANGLES, 0, 6);
      frame = requestAnimationFrame(render);
    }

    function resetPointer() {
      targetX = 0;
      targetY = 0;
    }

    a4Sheet.addEventListener("pointermove", handlePointer);
    a4Sheet.addEventListener("pointerleave", resetPointer);
    frame = requestAnimationFrame(render);

    return () => {
      cancelAnimationFrame(frame);
      a4Sheet.removeEventListener("pointermove", handlePointer);
      a4Sheet.removeEventListener("pointerleave", resetPointer);
      gl.deleteBuffer(position);
      gl.deleteProgram(program);
    };
  });
</script>

<svelte:head>
  <title>Text shader — Range preview</title>
  <meta
    name="description"
    content="An interactive embossed text material study for Range."
  />
  <meta name="robots" content="noindex, nofollow" />
</svelte:head>

<main class="shaderPage">
  <div class="grain" aria-hidden="true"></div>

  <header>
    <span class="eyebrow">Range material study</span>
    <span class="directions" aria-label="Light directions: 90 and 270 degrees">
      90° <i></i> 270°
    </span>
  </header>

  <section class="stage" aria-live="polite">
    <div class="a4Sheet" bind:this={a4Sheet}>
      <canvas
        class="a4Shader"
        bind:this={a4Canvas}
        aria-label="Distorted amber and blue spotlight fields with a pure-white lower gradient blending into burnt orange"
      ></canvas>
      <span class="sheetLabel">A4 · 210 × 297</span>
      <svg class="shader" viewBox="0 0 1400 560" role="img" aria-label={displayText}>
      <defs>
        <filter
          id="range-text-material"
          x="-18%"
          y="-30%"
          width="136%"
          height="160%"
          color-interpolation-filters="sRGB"
        >
          <feMorphology in="SourceAlpha" operator="dilate" radius={outer90.spread} result="outer-90-spread" />
          <feGaussianBlur in="outer-90-spread" stdDeviation={outer90.blur} result="outer-90-soft" />
          <feOffset
            in="outer-90-soft"
            dx={offset(outer90.angle, outer90.distance).x}
            dy={offset(outer90.angle, outer90.distance).y}
            result="outer-90-offset"
          />
          <feFlood flood-color={outer90.color} flood-opacity={outer90.opacity / 100} result="outer-90-color" />
          <feComposite
            in="outer-90-color"
            in2="outer-90-offset"
            operator="in"
            result="outer-90-shadow"
          />

          <feMorphology in="SourceAlpha" operator="dilate" radius={outer270.spread} result="outer-270-spread" />
          <feGaussianBlur in="outer-270-spread" stdDeviation={outer270.blur} result="outer-270-soft" />
          <feOffset
            in="outer-270-soft"
            dx={offset(outer270.angle, outer270.distance).x}
            dy={offset(outer270.angle, outer270.distance).y}
            result="outer-270-offset"
          />
          <feFlood flood-color={outer270.color} flood-opacity={outer270.opacity / 100} result="outer-270-color" />
          <feComposite
            in="outer-270-color"
            in2="outer-270-offset"
            operator="in"
            result="outer-270-shadow"
          />

          <feMorphology in="SourceAlpha" operator="erode" radius={inner90.spread} result="inner-90-spread" />
          <feGaussianBlur in="inner-90-spread" stdDeviation={inner90.blur} result="inner-90-soft" />
          <feOffset
            in="inner-90-soft"
            dx={offset(inner90.angle, inner90.distance, true).x}
            dy={offset(inner90.angle, inner90.distance, true).y}
            result="inner-90-offset"
          />
          <feComposite
            in="SourceAlpha"
            in2="inner-90-offset"
            operator="out"
            result="inner-90-mask"
          />
          <feFlood flood-color={inner90.color} flood-opacity={inner90.opacity / 100} result="inner-90-color" />
          <feComposite
            in="inner-90-color"
            in2="inner-90-mask"
            operator="in"
            result="inner-90-shadow"
          />

          <feMorphology in="SourceAlpha" operator="erode" radius={inner270.spread} result="inner-270-spread" />
          <feGaussianBlur in="inner-270-spread" stdDeviation={inner270.blur} result="inner-270-soft" />
          <feOffset
            in="inner-270-soft"
            dx={offset(inner270.angle, inner270.distance, true).x}
            dy={offset(inner270.angle, inner270.distance, true).y}
            result="inner-270-offset"
          />
          <feComposite
            in="SourceAlpha"
            in2="inner-270-offset"
            operator="out"
            result="inner-270-mask"
          />
          <feFlood flood-color={inner270.color} flood-opacity={inner270.opacity / 100} result="inner-270-color" />
          <feComposite
            in="inner-270-color"
            in2="inner-270-mask"
            operator="in"
            result="inner-270-shadow"
          />

          <feMerge>
            <feMergeNode in="outer-270-shadow" />
            <feMergeNode in="outer-90-shadow" />
            <feMergeNode in="SourceGraphic" />
            <feMergeNode in="inner-270-shadow" />
            <feMergeNode in="inner-90-shadow" />
          </feMerge>
        </filter>
      </defs>

      <text
        class="shaderText"
        x="700"
        y="306"
        text-anchor="middle"
        dominant-baseline="middle"
        fill={baseColor}
        textLength={Math.min(1120, Math.max(420, displayText.length * 178))}
        lengthAdjust="spacingAndGlyphs"
      >{displayText}</text>
      </svg>
      {#if !shaderAvailable}
        <span class="shaderFallback">Shader unavailable</span>
      {/if}
    </div>
  </section>

  <form onsubmit={(event) => event.preventDefault()}>
    <div class="textControl">
      <label class="controlLabel" for="shader-copy">Text</label>
      <div class="field">
      <input
        class="textInput"
        id="shader-copy"
        bind:value={sample}
        maxlength="12"
        autocomplete="off"
        spellcheck="false"
        aria-describedby="character-count"
      />
      <span id="character-count">{sample.length}/12</span>
      </div>
    </div>

    <div class="materialHeader">
      <span>Material controls</span>
      <label class="baseControl">
        <input type="color" bind:value={baseColor} aria-label="Base glyph color" />
        <span>Base</span>
        <code>{baseColor}</code>
      </label>
    </div>

    <div class="shadowControls">
      {#each [inner90, outer90, inner270, outer270] as shadow}
        <fieldset class="shadowControl">
          <legend>
            <input type="color" bind:value={shadow.color} aria-label={`${shadow.label} color`} />
            <span>{shadow.label}</span>
            <code>{shadow.color}</code>
          </legend>

          <label class="rangeControl">
            <span>Rotation</span>
            <output>{shadow.angle}°</output>
            <input type="range" min="0" max="360" step="1" bind:value={shadow.angle} />
          </label>
          <label class="rangeControl">
            <span>Distance</span>
            <output>{shadow.distance}px</output>
            <input type="range" min="0" max="40" step="1" bind:value={shadow.distance} />
          </label>
          <label class="rangeControl">
            <span>Blur</span>
            <output>{shadow.blur}px</output>
            <input type="range" min="0" max="36" step="0.5" bind:value={shadow.blur} />
          </label>
          <label class="rangeControl">
            <span>Spread</span>
            <output>{shadow.spread}px</output>
            <input type="range" min="0" max="12" step="0.5" bind:value={shadow.spread} />
          </label>
          <label class="rangeControl">
            <span>Intensity</span>
            <output>{shadow.opacity}%</output>
            <input type="range" min="0" max="100" step="1" bind:value={shadow.opacity} />
          </label>
        </fieldset>
      {/each}
    </div>
  </form>
</main>

<style>
  :global(html) {
    background: #ffffff;
  }

  :global(body) {
    overflow: hidden;
    background: #ffffff;
  }

  .shaderPage {
    position: relative;
    isolation: isolate;
    width: 100%;
    min-height: 100svh;
    margin: 0;
    padding: clamp(22px, 3vw, 42px);
    display: grid;
    grid-template-rows: auto 1fr auto;
    overflow: hidden;
    color: #292b30;
    background: #ffffff;
  }

  .shaderPage::before {
    position: absolute;
    z-index: -1;
    inset: 0;
    box-shadow: inset 0 0 14vw rgba(34, 36, 42, 0.025);
    content: "";
    pointer-events: none;
  }

  .grain {
    position: absolute;
    z-index: -1;
    inset: 0;
    opacity: 0.19;
    pointer-events: none;
    background-image: url("data:image/svg+xml,%3Csvg viewBox='0 0 180 180' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='.82' numOctaves='3' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)' opacity='.18'/%3E%3C/svg%3E");
    mix-blend-mode: soft-light;
  }

  header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 24px;
    font-family: var(--font-geist-mono), monospace;
    font-size: 10px;
    letter-spacing: 0.12em;
    line-height: 1;
    text-transform: uppercase;
  }

  .eyebrow {
    color: rgba(30, 32, 37, 0.62);
  }

  .directions {
    display: flex;
    align-items: center;
    gap: 10px;
    color: rgba(30, 32, 37, 0.44);
  }

  .directions i {
    width: 28px;
    height: 1px;
    display: block;
    background: currentColor;
  }

  .stage {
    min-height: 0;
    display: grid;
    align-items: end;
    justify-items: center;
    padding: 18px 0;
  }

  .a4Sheet {
    position: relative;
    isolation: isolate;
    height: min(43vh, 520px);
    aspect-ratio: 210 / 297;
    display: grid;
    place-items: center;
    border: 1px solid rgba(30, 32, 37, 0.1);
    background: #ffffff;
    box-shadow:
      0 18px 54px rgba(30, 32, 37, 0.08),
      0 2px 8px rgba(30, 32, 37, 0.05);
    overflow: hidden;
    touch-action: none;
  }

  .a4Shader {
    position: absolute;
    z-index: 0;
    inset: 0;
    width: 100%;
    height: 100%;
    display: block;
  }

  .sheetLabel {
    position: absolute;
    z-index: 3;
    top: 13px;
    right: 14px;
    color: rgba(30, 32, 37, 0.3);
    font-family: var(--font-geist-mono), monospace;
    font-size: 8px;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .shader {
    position: absolute;
    z-index: 2;
    top: 86%;
    left: 50%;
    width: 88%;
    overflow: visible;
    transform: translate(-50%, -50%);
  }

  .shaderFallback {
    position: absolute;
    z-index: 1;
    inset: 0;
    display: grid;
    place-items: center;
    color: rgba(30, 32, 37, 0.44);
    background: #f6ead5;
    font-family: var(--font-geist-mono), monospace;
    font-size: 9px;
    text-transform: uppercase;
  }

  .shaderText {
    filter: url(#range-text-material);
    font-family: var(--font-range-sans), sans-serif;
    font-size: 256px;
    font-weight: 680;
    letter-spacing: -0.075em;
  }

  form {
    width: min(1120px, 100%);
    margin: 0 auto;
    display: grid;
    gap: 12px;
  }

  .controlLabel,
  .materialHeader > span {
    display: block;
    margin: 0 0 9px 2px;
    color: rgba(30, 32, 37, 0.52);
    font-family: var(--font-geist-mono), monospace;
    font-size: 10px;
    letter-spacing: 0.11em;
    line-height: 1;
    text-transform: uppercase;
  }

  fieldset {
    min-width: 0;
    margin: 0;
    padding: 0;
    border: 0;
  }

  .textControl {
    width: min(460px, 100%);
  }

  .field {
    height: 52px;
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 0 16px;
    border: 1px solid rgba(255, 255, 255, 0.66);
    border-radius: 14px;
    background: rgba(220, 221, 223, 0.7);
    box-shadow:
      0 8px 28px rgba(37, 39, 44, 0.07),
      inset 0 1px 1px rgba(255, 255, 255, 0.82),
      inset 0 -1px 1px rgba(37, 39, 44, 0.1);
    backdrop-filter: blur(16px);
  }

  .textInput {
    min-width: 0;
    flex: 1;
    padding: 0;
    border: 0;
    outline: 0;
    color: #292b30;
    background: transparent;
    font: 500 16px/1 var(--font-range-sans), sans-serif;
    letter-spacing: -0.02em;
  }

  .materialHeader {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
  }

  .materialHeader > span {
    margin: 0 0 0 2px;
  }

  .baseControl {
    display: flex;
    align-items: center;
    gap: 7px;
    color: rgba(30, 32, 37, 0.6);
    font-size: 10px;
    cursor: pointer;
  }

  .baseControl code,
  .shadowControl code {
    color: rgba(30, 32, 37, 0.38);
    font-family: var(--font-geist-mono), monospace;
    font-size: 9px;
    text-transform: uppercase;
  }

  input[type="color"] {
    width: 24px;
    height: 24px;
    flex: 0 0 24px;
    padding: 0;
    border: 0;
    border-radius: 7px;
    outline: 0;
    overflow: hidden;
    background: transparent;
    cursor: pointer;
  }

  input[type="color"]::-webkit-color-swatch-wrapper {
    padding: 0;
  }

  input[type="color"]::-webkit-color-swatch {
    border: 1px solid rgba(30, 32, 37, 0.13);
    border-radius: 6px;
  }

  .shadowControls {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: 8px;
  }

  .shadowControl {
    padding: 12px;
    border: 1px solid rgba(30, 32, 37, 0.08);
    border-radius: 14px;
    background: rgba(238, 238, 239, 0.78);
    box-shadow:
      0 8px 24px rgba(37, 39, 44, 0.035),
      inset 0 1px 1px rgba(255, 255, 255, 0.9);
  }

  .shadowControl legend {
    width: 100%;
    margin: 0 0 9px;
    padding: 0;
    display: flex;
    align-items: center;
    gap: 7px;
    color: rgba(30, 32, 37, 0.7);
    font-size: 10px;
    font-weight: 570;
  }

  .shadowControl legend span {
    min-width: 0;
    flex: 1;
  }

  .rangeControl {
    display: grid;
    grid-template-columns: 1fr auto;
    align-items: center;
    gap: 3px 8px;
    padding: 3px 0;
    color: rgba(30, 32, 37, 0.55);
    font-family: var(--font-geist-mono), monospace;
    font-size: 9px;
    line-height: 1;
  }

  .rangeControl output {
    min-width: 34px;
    color: rgba(30, 32, 37, 0.72);
    text-align: right;
  }

  .rangeControl input[type="range"] {
    width: 100%;
    height: 14px;
    grid-column: 1 / -1;
    margin: 0;
    accent-color: #34363b;
    cursor: ew-resize;
  }

  .field:focus-within {
    border-color: rgba(255, 255, 255, 0.94);
    box-shadow:
      0 10px 34px rgba(37, 39, 44, 0.09),
      0 0 0 3px rgba(255, 255, 255, 0.22),
      inset 0 1px 1px white,
      inset 0 -1px 1px rgba(37, 39, 44, 0.11);
  }

  #character-count {
    color: rgba(30, 32, 37, 0.36);
    font-family: var(--font-geist-mono), monospace;
    font-size: 10px;
  }

  @media (max-width: 640px) {
    :global(body) {
      overflow: auto;
    }

    .shaderPage {
      padding: 20px 16px 24px;
      overflow: visible;
    }

    .shader {
      width: 90%;
    }

    .shaderText {
      font-size: 224px;
    }

    .shadowControls {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 420px) {
    .shadowControls {
      grid-template-columns: 1fr;
    }
  }

  @media (prefers-reduced-motion: no-preference) {
    .shaderText {
      animation: settle 900ms cubic-bezier(0.16, 1, 0.3, 1) both;
    }

    @keyframes settle {
      from {
        opacity: 0;
        transform: translateY(10px);
      }
    }
  }
</style>
