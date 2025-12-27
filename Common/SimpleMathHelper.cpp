#include "pch.h"
#include "SimpleMathHelper.h"

namespace Math
{
	int cn_PnPoly(D2D1_POINT_2F P, std::vector<D2D1_POINT_2F> V, int n)
	{
		int cn = 0;

		for (int i = 0; i < n; i++)
		{
			if (((V[i].y <= P.y) && (V[i + 1].y > P.y))
				|| ((V[i].y > P.y) && V[i + 1].y <= P.y))
			{
				float vt = (float)(P.y - V[i].y) / (V[i + 1].y - V[i].y);
				if (P.x < V[i].x + vt * (V[i + 1].x - V[i].x))
				{
					cn++;
				}
			}
		}

		return cn;
	}

	int wn_PnPoly(D2D1_POINT_2F P, std::vector<D2D1_POINT_2F> V, int n)
	{
		int wn = 0;

		for (int i = 0; i < n; i++)
		{
			if (V[i].y <= P.y)
			{
				if (V[i + 1].y > P.y)
				{
					if (IsLeft(V[i], V[i + 1], P) > 0)
					{
						++wn;
					}
				}
			}
			else
			{
				if (V[i + 1].y <= P.y)
				{
					if (IsLeft(V[i], V[i + 1], P) < 0)
					{
						++wn;
					}
				}
			}
		}

		return wn;
	}

	bool is_circum(Triangle cur, int i, std::vector<Vector2F>& point)
	{
		float ccw = (point[cur.b] - point[cur.a]).Cross(point[cur.c] - point[cur.a]);

		float adx = point[cur.a].x - point[i].x;
		float ady = point[cur.a].y - point[i].y;
		float bdx = point[cur.b].x - point[i].x;
		float bdy = point[cur.b].y - point[i].y;
		float cdx = point[cur.c].x - point[i].x;
		float cdy = point[cur.c].y - point[i].y;
		float bdxcdy = bdx * cdy, cdxbdy = cdx * bdy;
		float cdxady = cdx * ady, adxcdy = adx * cdy;
		float adxbdy = adx * bdy, bdxady = bdx * ady;
		float alift = adx * adx + ady * ady;
		float blift = bdx * bdx + bdy * bdy;
		float clift = cdx * cdx + cdy * cdy;

		float det = alift * (bdxcdy - cdxbdy)
			+ blift * (cdxady - adxcdy)
			+ clift * (adxbdy - bdxady);

		if (ccw > 0) return det >= 0;
		else return det <= 0;
	}
}