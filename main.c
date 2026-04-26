#define _GNU_SOURCE
#include "lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static const char *HTML =
"<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Lattice</title><style>body{font-family:system-ui;margin:0;background:#111827;color:#f8fafc}main{max-width:1050px;margin:auto;padding:28px}.card{background:#1f2937;border:1px solid #374151;border-radius:18px;padding:20px;margin:16px 0;box-shadow:0 20px 60px #0005}textarea{width:100%;min-height:120px;background:#0f172a;color:#e5e7eb;border:1px solid #475569;border-radius:12px;padding:12px}button{background:#38bdf8;color:#082f49;border:0;border-radius:999px;padding:12px 18px;font-weight:800;cursor:pointer}.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}.metric{font-size:34px;font-weight:900;color:#a7f3d0}.defect{color:#fca5a5}.phase{color:#fde68a}svg{width:100%;height:320px;background:#020617;border-radius:14px}.cell{fill:#38bdf8}.weak{fill:#fbbf24}.bad{fill:#ef4444}.link{stroke:#64748b;stroke-width:2}.hint{color:#94a3b8}@media(max-width:800px){.grid{grid-template-columns:1fr}}</style></head>"
"<body><main><h1>◇ Lattice</h1><p class='hint'>Crystallography-inspired spaced repetition: unit cells, lattice planes, red defects, and phase-transition learning milestones.</p>"
"<div class='grid'><div class='card'><h2>Unit cells</h2><p class='hint'>One per line: concept | mastery 0-1 | defect 0/1</p><textarea id='concepts'></textarea></div>"
"<div class='card'><h2>Lattice planes</h2><p class='hint'>One per line: from_index-&gt;to_index | relationship</p><textarea id='relations'></textarea></div></div>"
"<button onclick='analyze()'>Run X-ray diffraction</button> <button onclick='loadSample()'>Load sample crystal</button>"
"<div class='card'><h2>Crystal readout</h2><div id='readout'></div><svg id='viz'></svg></div>"
"<script>async function loadSample(){let r=await fetch('/api/sample');let s=await r.json();concepts.value=s.concepts;relations.value=s.relations;analyze()}"
"function enc(s){return encodeURIComponent(s)} async function analyze(){let body='concepts='+enc(concepts.value)+'&relations='+enc(relations.value);let r=await fetch('/api/analyze',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});let a=await r.json();readout.innerHTML=`<div class=grid><div><div class=metric>${a.crystallization_score.toFixed(1)}</div><b>Crystallization score</b></div><div><div class=metric>${Math.round(a.structural_density*100)}%</div><b>Structural density</b></div><div><span class='${a.defects?'defect':''}'>Defects: ${a.defects}</span></div><div><span class='phase'>Phase transition: ${a.phase_transition_ready?'READY':'not yet'}</span></div></div><p><b>Frame:</b> ${a.bravais_frame}</p><p><b>Next X-ray review:</b> ${a.next_review||'none'}</p><p>${a.recommendation}</p>`;draw(a)}"
"function draw(a){let lines=concepts.value.split(/\n|;/).filter(x=>x.trim());let rel=relations.value.split(/\n|;/).filter(x=>x.trim());let svg=document.getElementById('viz');svg.innerHTML='';let W=900,H=320,cx=W/2,cy=H/2,R=Math.min(120,40+lines.length*12);let pts=lines.map((l,i)=>{let ang=2*Math.PI*i/Math.max(1,lines.length);return [cx+R*Math.cos(ang),cy+R*Math.sin(ang),l]});rel.forEach(x=>{let m=x.match(/(\\d+)\\s*->\\s*(\\d+)/);if(m&&pts[m[1]]&&pts[m[2]])svg.innerHTML+=`<line class=link x1=${pts[m[1]][0]} y1=${pts[m[1]][1]} x2=${pts[m[2]][0]} y2=${pts[m[2]][1]}/>`});pts.forEach((p,i)=>{let parts=p[2].split('|'), mastery=parseFloat(parts[1]||.5), defect=parseInt(parts[2]||0);let cls=defect?'bad':mastery<.55?'weak':'cell';svg.innerHTML+=`<circle class=${cls} cx=${p[0]} cy=${p[1]} r=17 /><text x=${p[0]+22} y=${p[1]+5} fill=#e5e7eb font-size=13>${i}: ${parts[0].trim()}</text>`})}loadSample()</script></main></body></html>";

static void usage(const char *prog) {
    printf("Usage: %s [--serve [PORT]] [--sample] [--help]\n", prog);
    printf("C web app for crystallography-inspired knowledge lattice analysis.\n");
}

static void http_reply(int fd, const char *status, const char *type, const char *body) {
    char header[512];
    int len = (int)strlen(body);
    snprintf(header, sizeof(header), "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\n\r\n", status, type, len);
    send(fd, header, strlen(header), 0);
    send(fd, body, len, 0);
}

static void url_decode(char *s) {
    char *o = s;
    for (char *p = s; *p; ++p) {
        if (*p == '+') *o++ = ' ';
        else if (*p == '%' && isxdigit((unsigned char)p[1]) && isxdigit((unsigned char)p[2])) {
            char hex[3] = {p[1], p[2], 0}; *o++ = (char)strtol(hex, NULL, 16); p += 2;
        } else *o++ = *p;
    }
    *o = 0;
}

static void form_value(const char *body, const char *key, char *out, size_t n) {
    out[0] = 0;
    size_t k = strlen(key);
    const char *p = body;
    while (p && *p) {
        if (strncmp(p, key, k) == 0 && p[k] == '=') {
            p += k + 1; const char *e = strchr(p, '&'); size_t len = e ? (size_t)(e - p) : strlen(p);
            if (len >= n) len = n - 1;
            memcpy(out, p, len);
            out[len] = 0;
            url_decode(out);
            return;
        }
        p = strchr(p, '&'); if (p) p++;
    }
}

static void handle_client(int fd) {
    char req[20000];
    int n = (int)recv(fd, req, sizeof(req)-1, 0);
    if (n <= 0) return;
    req[n] = 0;
    if (strncmp(req, "GET / ", 6) == 0 || strncmp(req, "GET /HTTP", 9) == 0) { http_reply(fd, "200 OK", "text/html; charset=utf-8", HTML); return; }
    if (strncmp(req, "GET /api/sample", 15) == 0) {
        const char *json = "{\"concepts\":\"Spaced repetition interval|0.82|0\\nRetrieval practice|0.74|0\\nInterleaving|0.51|0\\nMisconception: rereading equals learning|0.28|1\\nTransfer to new problems|0.67|0\",\"relations\":\"0->1|cause\\n1->2|reinforces\\n2->4|enables\\n3->1|opposite\\n0->4|supports\"}";
        http_reply(fd, "200 OK", "application/json", json); return;
    }
    if (strncmp(req, "POST /api/analyze", 17) == 0) {
        char *body = strstr(req, "\r\n\r\n"); body = body ? body + 4 : "";
        char concepts[8192], relations[8192], json[4096];
        form_value(body, "concepts", concepts, sizeof(concepts));
        form_value(body, "relations", relations, sizeof(relations));
        KnowledgeCrystal c; parse_crystal_text(concepts, relations, &c);
        LatticeAnalysis a = crystal_analyze(&c);
        analysis_to_json(&c, &a, json, sizeof(json));
        http_reply(fd, "200 OK", "application/json", json); return;
    }
    http_reply(fd, "404 Not Found", "text/plain", "not found\n");
}

static int serve(int port) {
    signal(SIGPIPE, SIG_IGN);
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }
    int yes = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_ANY); addr.sin_port = htons((unsigned short)port);
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(s); return 1; }
    if (listen(s, 16) < 0) { perror("listen"); close(s); return 1; }
    printf("Lattice web app listening on http://127.0.0.1:%d\n", port); fflush(stdout);
    while (1) { int fd = accept(s, NULL, NULL); if (fd >= 0) { handle_client(fd); close(fd); } }
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) { usage(argv[0]); return 0; }
    if (argc > 1 && strcmp(argv[1], "--sample") == 0) {
        KnowledgeCrystal c; char text[2048]; crystal_sample(&c); LatticeAnalysis a = crystal_analyze(&c); analysis_to_text(&c, &a, text, sizeof(text)); puts(text); return 0;
    }
    int port = 9879;
    const char *env = getenv("PORT"); if (env && *env) port = atoi(env);
    if (argc > 2 && strcmp(argv[1], "--serve") == 0) port = atoi(argv[2]);
    return serve(port > 0 ? port : 9879);
}
