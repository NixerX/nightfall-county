// Nightfall County: static merge.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// merge static (L_ALWAYS) props of each 32m cell into one vertex-colored mesh
static void appendMesh(std::vector<float>& V, std::vector<float>& N, std::vector<unsigned char>& Cc, const Mesh& m,
                       const Prop& p) {
    Matrix M = TRS(p.p, p.s, p.yaw);
    float cy = cosf(-p.yaw), sy = sinf(-p.yaw);
    int n = m.indices ? m.triangleCount * 3 : m.vertexCount;
    for (int k = 0; k < n; k++) {
        int i = m.indices ? m.indices[k] : k;
        Vector3 v = Vector3Transform(V3(m.vertices[i * 3], m.vertices[i * 3 + 1], m.vertices[i * 3 + 2]), M);
        Vector3 nn = V3(m.normals[i * 3] / p.s.x, m.normals[i * 3 + 1] / p.s.y, m.normals[i * 3 + 2] / p.s.z);
        nn = Vector3Normalize(V3(cy * nn.x + sy * nn.z, nn.y, -sy * nn.x + cy * nn.z));
        V.push_back(v.x);
        V.push_back(v.y);
        V.push_back(v.z);
        N.push_back(nn.x);
        N.push_back(nn.y);
        N.push_back(nn.z);
        Cc.push_back(p.c.r);
        Cc.push_back(p.c.g);
        Cc.push_back(p.c.b);
        Cc.push_back(255);
    }
}
static void buildChunks() {
    for (int x = 0; x < GN; x++)
        for (int z = 0; z < GN; z++) {
            if (chunkHas[x][z])
                UnloadMesh(chunkMesh[x][z]);
            chunkHas[x][z] = false;
        }
    for (int x = 0; x < GN; x++)
        for (int z = 0; z < GN; z++) {
            std::vector<float> V, N;
            std::vector<unsigned char> Cc;
            for (int pi : pgrid[x][z]) {
                const Prop& p = props[pi];
                if (p.layer != L_ALWAYS)
                    continue;
                if (p.asset >= 0) {
                    Asset& a = assets[p.asset];
                    Matrix M = MatrixMultiply(a.m.transform, placeM(p.asset, p.p, p.s.x, p.yaw));
                    Matrix NM = MatrixTranspose(MatrixInvert(M));
                    for (int mi = 0; mi < a.m.meshCount; mi++) {
                        const Mesh& m = a.m.meshes[mi];
                        Color col0 = a.base[a.m.meshMaterial[mi]];
                        col0 = rgba(col0.r * p.c.r / 255, col0.g * p.c.g / 255, col0.b * p.c.b / 255);
                        int n = m.indices ? m.triangleCount * 3 : m.vertexCount;
                        bool tx = a.tex.data && m.texcoords;
                        const Color* tp = tx ? (const Color*)a.tex.data : nullptr;
                        for (int k = 0; k < n; k++) {
                            int i = m.indices ? m.indices[k] : k;
                            Vector3 v = Vector3Transform(
                                V3(m.vertices[i * 3], m.vertices[i * 3 + 1], m.vertices[i * 3 + 2]), M);
                            Color col = col0;
                            if (tx) {
                                int u = (int)(fmodf(fmodf(m.texcoords[i * 2], 1) + 1, 1) * (a.tex.width - 1)),
                                    w = (int)(fmodf(fmodf(m.texcoords[i * 2 + 1], 1) + 1, 1) * (a.tex.height - 1));
                                Color t = tp[w * a.tex.width + u];
                                col = rgba(col0.r * t.r / 255, col0.g * t.g / 255, col0.b * t.b / 255);
                            }
                            Vector3 nn = m.normals ? V3(m.normals[i * 3], m.normals[i * 3 + 1], m.normals[i * 3 + 2])
                                                   : V3(0, 1, 0);
                            nn = Vector3Normalize(V3(NM.m0 * nn.x + NM.m4 * nn.y + NM.m8 * nn.z,
                                                     NM.m1 * nn.x + NM.m5 * nn.y + NM.m9 * nn.z,
                                                     NM.m2 * nn.x + NM.m6 * nn.y + NM.m10 * nn.z));
                            V.push_back(v.x);
                            V.push_back(v.y);
                            V.push_back(v.z);
                            N.push_back(nn.x);
                            N.push_back(nn.y);
                            N.push_back(nn.z);
                            Cc.push_back(col.r);
                            Cc.push_back(col.g);
                            Cc.push_back(col.b);
                            Cc.push_back(255);
                        }
                    }
                    continue;
                }
                appendMesh(V, N, Cc, meshes[p.mesh], p);
            }
            if (V.empty())
                continue;
            Mesh m{};
            m.vertexCount = (int)V.size() / 3;
            m.triangleCount = m.vertexCount / 3;
            m.vertices = (float*)MemAlloc(V.size() * sizeof(float));
            memcpy(m.vertices, V.data(), V.size() * sizeof(float));
            m.normals = (float*)MemAlloc(N.size() * sizeof(float));
            memcpy(m.normals, N.data(), N.size() * sizeof(float));
            m.colors = (unsigned char*)MemAlloc(Cc.size());
            memcpy(m.colors, Cc.data(), Cc.size());
            m.texcoords = (float*)MemAlloc(m.vertexCount * 2 * sizeof(float));
            memset(m.texcoords, 0, m.vertexCount * 2 * sizeof(float));
            UploadMesh(&m, false);
            chunkMesh[x][z] = m;
            chunkHas[x][z] = true;
        }
}
