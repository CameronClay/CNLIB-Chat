#pragma once
#include "CNLIB\TCPServInterface.h"
#include "WhiteboardClientData.h"
#include "Palette.h"

class Whiteboard
{
public:
	Whiteboard(TCPServInterface &serv, WBParams params, std::tstring creator);
	Whiteboard(Whiteboard &&wb);

	~Whiteboard();

	void StartThread();

	void SendBitmap(RectU& rect);
	void SendBitmap(RectU& rect, const Socket& sock, bool single);

	void Draw();

	bool IsCreator(const std::tstring& user) const;

	void AddClient(Socket pc);
	void RemoveClient(Socket pc);

	const WBParams& GetParams() const;
	const Palette& GetPalette() const;
	WBClientData& GetClientData(Socket pc);
	BYTE* GetBitmap();
	HANDLE GetTimer() const;
	CRITICAL_SECTION* GetMapSect();
	std::unordered_map<Socket, WBClientData, Socket::Hash>& GetMap();
	std::vector<Socket>& GetPcs();

private:
	inline void PutPixel(int x, int y, BYTE clr)
	{
		pixels[x + (y * params.width)] = clr;
	}

	void DrawFlatTriangle(float y0, float y1, float m0, float b0, float m1, float b1, BYTE clr);
	void DrawTriangle(Vec2 v0, Vec2 v1, Vec2 v2, BYTE clr);

	void PaintBrush(WBClientData& clientData);
	void DrawLine(int x1, int y1, int x2, int y2, BYTE clr);
	void DrawQuadrilateral(const Vec2* vertices, BYTE clr);

	Vec2 ModifyPoint(Vec2 vect);

	UINT GetBufferLen(const RectU& rec) const;
	void MakeRectPixels(RectU& rect, char* ptr);
private:
	BYTE *pixels;
	WBParams params;
	CRITICAL_SECTION mapSect;
	TCPServInterface &serv;
	Palette palette;
	std::tstring creator;

	const float interval;
	HANDLE timer, thread;
	DWORD threadID;

	std::unordered_map<Socket, WBClientData, Socket::Hash> clients;
	std::vector<Socket> sendPcs;
};
