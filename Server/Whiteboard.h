#pragma once
#include "CNLIB\TCPServInterface.h"
#include <unordered_map>
#include "ColorDef.h"
#include "WhiteboardClientData.h"
#include "Palette.h"
#include "Timer.h"
#include "Vec2.h"

class Whiteboard
{
public:
	Whiteboard(TCPServInterface &serv, WBParams params, std::tstring creator);
	Whiteboard(Whiteboard &&wb);

	~Whiteboard();

	void StartThread();

	void Draw();
	void SendBitmap(const RectU& rect);
	void SendBitmap(const RectU& rect, Socket& sock, bool single);
	void Frame();

	BYTE* GetBitmap();
	CRITICAL_SECTION* GetBitmapSection();
	CRITICAL_SECTION* GetMapSect();
	std::unordered_map<Socket, WBClientData, Socket::Hash>& GetMap();
	std::vector<Socket>& GetPcs();
	bool IsCreator(const std::tstring& user) const;
	void AddClient(Socket pc);
	void RemoveClient(Socket pc);
	const WBParams& GetParams() const;
	const Palette& GetPalette() const;
	HANDLE GetTimer() const;

private:
	void PutPixel(const PointU& point, BYTE clr);
	void PutPixel(USHORT x, USHORT y, BYTE clr);

	void DrawFlatTriangle(float y0, float y1, float m0, float b0, float m1, float b1, BYTE clr);
	void DrawTriangle(Vec2 v0, Vec2 v1, Vec2 v2, BYTE clr);


	void PaintBrush(WBClientData& clientData, BYTE clr);
	void DrawLine(const PointU& p1, const PointU& p2, BYTE clr);
	void DrawLineThick(const PointU& p1, const PointU& p2, float width, BYTE clr);
	RectU MakeRect(const PointU &p0, const PointU &p1);
	UINT GetBufferLen(const RectU& rec) const;
	void MakeRectPixels(const RectU& rect, char* ptr);

private:
	WBParams params;
	BYTE *pixels;
	CRITICAL_SECTION bitmapSect, mapSect;
	TCPServInterface &serv;
	Palette palette;
	std::tstring creator;

	const float interval;
	HANDLE timer, thread;
	DWORD threadID;

	std::unordered_map<Socket, WBClientData, Socket::Hash> clients;
	std::vector<Socket> sendPcs;
};
