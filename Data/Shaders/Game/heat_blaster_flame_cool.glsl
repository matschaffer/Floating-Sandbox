#include "heat_blaster_flame.glslinc"

void main()
{         
    float radius1 = GetFlameRadius(-1.0);
    
    // Focus (compress dynamics)
    float radius2 = smoothstep(0.2, 0.35, radius1);
    
    float alpha = 1.0 - radius2;

    vec3 col1 = mix(vec3(1.0, 1.0, 1.0), vec3(0.6, 1.0, 1.0), smoothstep(0.1, 0.2, radius1));
    col1 = mix(col1, vec3(0.1, 0.4, 1.0), smoothstep(0.18, 0.25, radius1));

    gl_FragColor = vec4(col1.xyz, alpha);
}