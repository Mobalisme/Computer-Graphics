// lava.frag
uniform sampler2D uLava1; // Lava01
uniform sampler2D uLava2; // Lava02
uniform float uTime;

varying vec2 vUV;

float luminance(vec3 c){
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main(){
    vec2 uv1 = vUV * 1.5 + vec2(uTime * 0.01, uTime * 0.005);
    vec2 uv2 = vUV * 3.0 + vec2(-uTime * 0.03, uTime * 0.02);

    vec3 flowSample = texture2D(uLava2, uv2).rgb;
    vec2 distort = (flowSample.rg - 0.5) * 0.03;

    uv1 += distort * 0.25;
    uv2 += distort;

    vec3 base = texture2D(uLava1, uv1).rgb;
    vec3 flow = texture2D(uLava2, uv2).rgb;

    vec3 albedo = base * 0.85 + flow * 0.45;

    float glowMask = smoothstep(0.55, 0.85, luminance(flow));

    float pulse = 0.75 + 0.25 * sin(uTime * 2.2);

    vec3 emissive = flow * glowMask * (1.6 * pulse);

    vec3 color = albedo + emissive;

    gl_FragColor = vec4(color, 1.0);
}
