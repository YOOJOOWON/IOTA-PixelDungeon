#include "stdafx.h"
#include "Gnoll.h"
#include "Player.h"
#include "Map.h"
#include "UI.h"

Gnoll::Gnoll()
{
}


Gnoll::~Gnoll()
{
}

HRESULT Gnoll::init(POINT point)
{
	//입력받은 좌표를 초기 위치로
	_point = point;

	//각 이미지 개별할당(MANAGER는 다 똑같아져버림)
	_stay = new image;
	_stay->init("Img\\Enemy\\gnoll_stay.bmp", 44, 60, 2, 2, true, RGB(255, 0, 255));
	_move = new image;
	_move->init("Img\\Enemy\\gnoll_move.bmp", 88, 60, 4, 2, true, RGB(255, 0, 255));
	_attack->init("Img\\Enemy\\gnoll_attack.bmp", 48, 60, 2, 2, true, RGB(255, 0, 255));
	_dead = new image;
	_dead->init("Img\\Enemy\\gnoll_dead.bmp", 84, 52, 3, 2, true, RGB(255, 0, 255));

	//초기 설정은 stay
	_image = _stay;

	//실제 그려줄 위치는 해당 타일 중심을 기준
	_pointX = _point.x * TILESIZE + TILESIZE / 2;
	_pointY = _point.y * TILESIZE + TILESIZE / 2;

	//타일 중심으로 이미지 크기만큼 렉트, 이걸로 그림그림
	_hitBox = RectMakeCenter(_pointX, _pointY, _image->getFrameWidth(), _image->getFrameHeight());
	_attBox = RectMakeCenter(_pointX, _pointY, 0, 0);

	//살아있음
	_isLive = true;
	//플레이어 발견 못함
	_findPlayer = false;

	//초기 방향은 랜덤으로
	int a = RND->getInt(2);
	if (a == 0) _right = true;
	else _right = false;

	//최초 프레임은 0, 오른쪽 보면 y=0, 왼쪽이면 y=1
	_currntFrameX = 0;
	if (_right) _currntFrameY = 0;
	else _currntFrameY = 1;

	//프레임 적용
	_image->setFrameX(_currntFrameX);
	_image->setFrameY(_currntFrameY);

	//내 턴 아님, 안움직임
	_action = false;
	_isMove = false;

	//스탯 설정
	_statistics.lv = 1;
	_statistics.maxLv = 8;
	_statistics.exp = 2;
	_statistics.hp = 12;
	_currntHp = 12;
	_statistics.avd_lck = 4;
	_statistics.def = 2;
	a = RND->getFromIntTo(2, 5);
	_statistics.str = a;
	_statistics.atk_lck = 11;

	//깨어있을지 자고있을지 랜덤 설정
	a = RND->getInt(2);
	if (a == 0) _myState = ENEMYSTATE_SLEEP;
	else _myState = ENEMYSTATE_IDLE;

	//초기값 설정
	_movePoint = PointMake(0, 0);
	_frameCount = 0;

	/*ENEMYSTATE_SLEEP,	//플레이어를 찾지 못한상태/수면상태
	ENEMYSTATE_IDLE,	//플레이어를 찾은 상태에서의 기본
	ENEMYSTATE_MOVE,
	ENEMYSTATE_ATTACK,
	ENEMYSTATE_END*/

	return S_OK;
}

void Gnoll::release()
{
	SAFE_RELEASE(_image);
	SAFE_DELETE(_image);

	SAFE_RELEASE(_stay);
	SAFE_DELETE(_stay);

	SAFE_RELEASE(_move);
	SAFE_DELETE(_move);

	SAFE_RELEASE(_attack);
	SAFE_DELETE(_attack);

	SAFE_RELEASE(_dead);
	SAFE_DELETE(_dead);
}


void Gnoll::getDamaged(int damage)
{
	_currntHp -= damage;
}

void Gnoll::draw(POINT camera)
{
	_image->frameRender(getMemDC(), _hitBox.left + camera.x, _hitBox.top + camera.y);
}


void Gnoll::frameUpdate()
{
	_frameCount++;

	if (_findPlayer)
	{
		if (_player->getPoint().x >= _point.x) _right = true;
		else _right = false;
	}
	if (_right) _currntFrameY = 0;
	else _currntFrameY = 1;

	if (_frameCount >= 10)
	{
		_frameCount = 0;
		switch (_myState)
		{
		case ENEMYSTATE_SLEEP:
			_image = _stay;
			_currntFrameX = 0;
			_image->setFrameX(_currntFrameX);
			_image->setFrameY(_currntFrameY);
			break;
		case ENEMYSTATE_IDLE:
			_image = _stay;
			_currntFrameX++;
			if (_currntFrameX > _image->getMaxFrameX()) _currntFrameX = 0;
			_image->setFrameX(_currntFrameX);
			_image->setFrameY(_currntFrameY);
			break;
		case ENEMYSTATE_MOVE:
			_image = _move;
			_currntFrameX++;
			if (_currntFrameX > _image->getMaxFrameX()) _currntFrameX = 0;
			_image->setFrameX(_currntFrameX);
			_image->setFrameY(_currntFrameY);
			break;
		case ENEMYSTATE_ATTACK:
			_image = _attack;
			_currntFrameX++;
			if (_currntFrameX > _image->getMaxFrameX())
			{
				_currntFrameX = 0;
				_myState = ENEMYSTATE_IDLE;
				_image = _stay;
			}
			_image->setFrameX(_currntFrameX);
			_image->setFrameY(_currntFrameY);
			break;
		}
	}

	_hitBox = RectMakeCenter(_pointX, _pointY, _image->getFrameWidth(), _image->getFrameHeight());
}

void Gnoll::action()
{
	if (_myState == ENEMYSTATE_SLEEP)
	{
		float dis = getDistance(_player->getPoint().x, _player->getPoint().y, _point.x, _point.y);

		if (dis < 2)
		{
			_myState = ENEMYSTATE_IDLE;
			_findPlayer = true;
			_action = false;
		}
		else
			_action = false;
	}
	else if (_myState == ENEMYSTATE_IDLE)
	{
		if (!_findPlayer)
		{
			//적을 발견하지 않았으면 랜덤행동

			float dis = getDistance(_player->getPoint().x, _player->getPoint().y, _point.x, _point.y);

			if (dis < 2)
			{
				//거리가 일정 범위 이내로 적이 들어왔으면 인식
				//인식한 턴은 그냥 자동으로 넘겨줌
				_myState = ENEMYSTATE_IDLE;
				_findPlayer = true;
				_action = false;
			}
			else
			{
				//거리가 멀면 랜덤이동
				if (!_isMove)
				{
					//0부터 시계방향으로, 8은 대기
					//해당 방향의 타일을 검사한 후에, 갈 수 있다면 그쪽으로 이동
					int a = RND->getInt(50);
					_myState = ENEMYSTATE_MOVE;
					switch (a)
					{
					case 0:
						//위
						if (_map->getMap(_point.x, _point.y - 1).obj == OBJ_NONE &&
							(_map->getMap(_point.x, _point.y - 1).terrain == TERRAIN_FLOOR ||
								_map->getMap(_point.x, _point.y - 1).terrain == TERRAIN_GRASS))
						{
							_isMove = true;
							_movePoint = PointMake(_point.x, _point.y - 1);
							_right = false;
						}
						break;
					case 1:
						//오른쪽 위
						if (_map->getMap(_point.x + 1, _point.y - 1).obj == OBJ_NONE &&
							(_map->getMap(_point.x + 1, _point.y - 1).terrain == TERRAIN_FLOOR ||
								_map->getMap(_point.x + 1, _point.y - 1).terrain == TERRAIN_GRASS))
						{
							_isMove = true;
							_movePoint = PointMake(_point.x + 1, _point.y - 1);
							_right = true;
						}
						break;
					case 2:
						//오른쪽
						if (_map->getMap(_point.x + 1, _point.y).obj == OBJ_NONE &&
							(_map->getMap(_point.x + 1, _point.y).terrain == TERRAIN_FLOOR ||
								_map->getMap(_point.x + 1, _point.y).terrain == TERRAIN_GRASS))
						{
							_isMove = true;
							_movePoint = PointMake(_point.x + 1, _point.y);
							_right = true;
						}
						break;
					case 3:
						//오른쪽 아래
						if (_map->getMap(_point.x + 1, _point.y + 1).obj == OBJ_NONE &&
							(_map->getMap(_point.x + 1, _point.y + 1).terrain == TERRAIN_FLOOR ||
								_map->getMap(_point.x + 1, _point.y + 1).terrain == TERRAIN_GRASS))
						{
							_isMove = true;
							_movePoint = PointMake(_point.x + 1, _point.y + 1);
							_right = true;
						}
						break;
					case 4:
						//아래
						if (_map->getMap(_point.x, _point.y + 1).obj == OBJ_NONE &&
							(_map->getMap(_point.x, _point.y + 1).terrain == TERRAIN_FLOOR ||
								_map->getMap(_point.x, _point.y + 1).terrain == TERRAIN_GRASS))
						{
							_isMove = true;
							_movePoint = PointMake(_point.x, _point.y + 1);
							_right = false;
						}
						break;
					case 5:
						//왼쪽 아래
						if (_map->getMap(_point.x - 1, _point.y + 1).obj == OBJ_NONE &&
							(_map->getMap(_point.x - 1, _point.y + 1).terrain == TERRAIN_FLOOR ||
								_map->getMap(_point.x - 1, _point.y + 1).terrain == TERRAIN_GRASS))
						{
							_isMove = true;
							_movePoint = PointMake(_point.x - 1, _point.y + 1);
							_right = false;
						}
						break;
					case 6:
						//왼쪽
						if (_map->getMap(_point.x - 1, _point.y).obj == OBJ_NONE &&
							(_map->getMap(_point.x - 1, _point.y).terrain == TERRAIN_FLOOR ||
								_map->getMap(_point.x - 1, _point.y).terrain == TERRAIN_GRASS))
						{
							_isMove = true;
							_movePoint = PointMake(_point.x - 1, _point.y);
							_right = false;
						}
						break;
					case 7:
						//왼쪽 위
						if (_map->getMap(_point.x - 1, _point.y - 1).obj == OBJ_NONE &&
							(_map->getMap(_point.x - 1, _point.y - 1).terrain == TERRAIN_FLOOR ||
								_map->getMap(_point.x - 1, _point.y - 1).terrain == TERRAIN_GRASS))
						{
							_isMove = true;
							_movePoint = PointMake(_point.x - 1, _point.y - 1);
							_right = false;
						}
						break;
					default:
						_action = false;
						_myState = ENEMYSTATE_IDLE;
						break;
					}
				}
			}
		}
		else
		{
			//적을 발견했으면 A*를 이용해 최적루트로 이동한다
			//A* 아직 안됐으니 미룸
		}
	}
	else if (_myState == ENEMYSTATE_MOVE)
	{
		//좌표가 주어졌으면 해당 좌표로 가야한다
		//중심좌표를 구한다
		float x = _movePoint.x * TILESIZE + TILESIZE / 2;
		float y = _movePoint.y * TILESIZE + TILESIZE / 2;

		//중심좌표에 도달했는지 확인한다
		if (static_cast<float>(_pointX) == x &&
			static_cast<float>(_pointY) == y)
		{
			//턴을 종료하고 넘겨준다
			_isMove = false;
			_myState = ENEMYSTATE_IDLE;
			_action = false;
		}
		else
		{
			//도달하지 않았으면 이동한다
			if (_pointX < x)
			{
				//현재 좌표가 가려는 좌표의 중심보다 작으면 +
				_pointX += TILESIZE / 8;
			}
			else if (_pointX > x)
			{
				_pointX -= TILESIZE / 8;
			}

			if (_pointY < y)
			{
				_pointY += TILESIZE / 8;
			}
			else if (_pointY > y)
			{
				_pointY -= TILESIZE / 8;
			}

		}
	}


	frameUpdate();
}