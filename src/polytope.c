#include <float.h>
#include "polytope.h"


void gjkPtInit(gjk_pt_t *pt)
{
    gjkListInit(&pt->vertices);
    gjkListInit(&pt->edges);
    gjkListInit(&pt->faces);
}

void gjkPtDestroy(gjk_pt_t *pt)
{
    gjk_pt_face_t *f, *f2;
    gjk_pt_edge_t *e, *e2;
    gjk_pt_vertex_t *v, *v2;

    // TODO: can be done efectively with only one test on emptiness of
    //       pt->faces
    // first delete all faces
    gjkListForEachEntrySafe(&pt->faces, f, f2, list){
        gjkPtDelFace(pt, f);
    }

    // delete all edges
    gjkListForEachEntrySafe(&pt->edges, e, e2, list){
        gjkPtDelEdge(pt, e);
    }

    // delete all vertices
    gjkListForEachEntrySafe(&pt->vertices, v, v2, list){
        gjkPtDelVertex(pt, v);
    }
}


gjk_pt_vertex_t *gjkPtAddVertex(gjk_pt_t *pt, const gjk_vec3_t *v)
{
    gjk_pt_vertex_t *vert;

    vert = GJK_ALLOC(gjk_pt_vertex_t);
    vert->type = GJK_PT_VERTEX;
    gjkVec3Copy(&vert->v, v);

    vert->dist = gjkVec3Len2(&vert->v);
    gjkVec3Copy(&vert->witness, &vert->v);

    gjkListInit(&vert->edges);

    // add vertex to list
    gjkListAppend(&pt->vertices, &vert->list);

    return vert;
}

gjk_pt_edge_t *gjkPtAddEdge(gjk_pt_t *pt, gjk_pt_vertex_t *v1,
                                          gjk_pt_vertex_t *v2)
{
    const gjk_vec3_t *a, *b;
    gjk_pt_edge_t *edge;

    edge = GJK_ALLOC(gjk_pt_edge_t);
    edge->type = GJK_PT_EDGE;
    edge->vertex[0] = v1;
    edge->vertex[1] = v2;
    edge->faces[0] = edge->faces[1] = NULL;

    a = &edge->vertex[0]->v;
    b = &edge->vertex[1]->v;
    edge->dist = gjkVec3PointSegmentDist2(gjk_vec3_origin, a, b, &edge->witness);

    gjkListAppend(&edge->vertex[0]->edges, &edge->vertex_list[0]);
    gjkListAppend(&edge->vertex[1]->edges, &edge->vertex_list[1]);

    gjkListAppend(&pt->edges, &edge->list);

    return edge;
}

gjk_pt_face_t *gjkPtAddFace(gjk_pt_t *pt, gjk_pt_edge_t *e1,
                                          gjk_pt_edge_t *e2,
                                          gjk_pt_edge_t *e3)
{
    const gjk_vec3_t *a, *b, *c;
    gjk_pt_face_t *face;
    gjk_pt_edge_t *e;
    size_t i;

    face = GJK_ALLOC(gjk_pt_face_t);
    face->type = GJK_PT_FACE;
    face->edge[0] = e1;
    face->edge[1] = e2;
    face->edge[2] = e3;

    // obtain triplet of vertices
    a = &face->edge[0]->vertex[0]->v;
    b = &face->edge[0]->vertex[1]->v;
    e = face->edge[1];
    if (e->vertex[0] != face->edge[0]->vertex[0]
            && e->vertex[0] != face->edge[0]->vertex[1]){
        c = &e->vertex[0]->v;
    }else{
        c = &e->vertex[1]->v;
    }
    face->dist = gjkVec3PointTriDist2(gjk_vec3_origin, a, b, c, &face->witness);


    for (i = 0; i < 3; i++){
        if (face->edge[i]->faces[0] == NULL){
            face->edge[i]->faces[0] = face;
        }else{
            face->edge[i]->faces[1] = face;
        }
    }

    gjkListAppend(&pt->faces, &face->list);

    return face;
}


void gjkPtRecomputeDistances(gjk_pt_t *pt)
{
    gjk_pt_vertex_t *v;
    gjk_pt_edge_t *e;
    gjk_pt_face_t *f;
    const gjk_vec3_t *a, *b, *c;
    double dist;

    gjkListForEachEntry(&pt->vertices, v, list){
        dist = gjkVec3Len2(&v->v);
        v->dist = dist;
        gjkVec3Copy(&v->witness, &v->v);
    }

    gjkListForEachEntry(&pt->edges, e, list){
        a = &e->vertex[0]->v;
        b = &e->vertex[1]->v;
        dist = gjkVec3PointSegmentDist2(gjk_vec3_origin, a, b, &e->witness);
        e->dist = dist;
    }

    gjkListForEachEntry(&pt->faces, f, list){
        // obtain triplet of vertices
        a = &f->edge[0]->vertex[0]->v;
        b = &f->edge[0]->vertex[1]->v;
        e = f->edge[1];
        if (e->vertex[0] != f->edge[0]->vertex[0]
                && e->vertex[0] != f->edge[0]->vertex[1]){
            c = &e->vertex[0]->v;
        }else{
            c = &e->vertex[1]->v;
        }

        dist = gjkVec3PointTriDist2(gjk_vec3_origin, a, b, c, &f->witness);
        f->dist = dist;
    }
}

gjk_pt_el_t *gjkPtNearest(gjk_pt_t *pt)
{
    double nearest_dist = DBL_MAX;
    gjk_pt_el_t *nearest = NULL;
    gjk_pt_vertex_t *v;
    gjk_pt_edge_t *e;
    gjk_pt_face_t *f;

    gjkListForEachEntry(&pt->vertices, v, list){
        if (v->dist < nearest_dist){
            nearest_dist = v->dist;
            nearest = (gjk_pt_el_t *)v;
        }
    }

    gjkListForEachEntry(&pt->edges, e, list){
        if (e->dist < nearest_dist){
            nearest_dist = e->dist;
            nearest = (gjk_pt_el_t *)e;
        }
    }

    gjkListForEachEntry(&pt->faces, f, list){
        if (f->dist < nearest_dist){
            nearest_dist = f->dist;
            nearest = (gjk_pt_el_t *)f;
        }
    }

    return nearest;
}

