/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "stdafx.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS(weapon_elite, CELITE)

void CELITE::Spawn(void)
{
	pev->classname = MAKE_STRING("weapon_elite");

	Precache();

	m_iId = WEAPON_ELITE;
	SET_MODEL(edict(), "models/w_elite.mdl");

	m_iDefaultAmmo = ELITE_DEFAULT_GIVE;
	m_flAccuracy = 0.88f;

	FallInit();
}

void CELITE::Precache(void)
{
	PRECACHE_MODEL("models/v_elite.mdl");
	PRECACHE_MODEL("models/w_elite.mdl");

	PRECACHE_SOUND("weapons/elite_fire.wav");
	PRECACHE_SOUND("weapons/elite_reloadstart.wav");
	PRECACHE_SOUND("weapons/elite_leftclipin.wav");
	PRECACHE_SOUND("weapons/elite_clipout.wav");
	PRECACHE_SOUND("weapons/elite_sliderelease.wav");
	PRECACHE_SOUND("weapons/elite_rightclipin.wav");
	PRECACHE_SOUND("weapons/elite_deploy.wav");

	m_iShell = PRECACHE_MODEL("models/pshell.mdl");

	m_usFireELITE_LEFT = PRECACHE_EVENT(1, "events/elite_left.sc");
	m_usFireELITE_RIGHT = PRECACHE_EVENT(1, "events/elite_right.sc");
}

int CELITE::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = MAX_AMMO_9MM;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = ELITE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 5;
	p->iId = m_iId = WEAPON_ELITE;
	p->iFlags = 0;
	p->iWeight = ELITE_WEIGHT;

	return 1;
}

BOOL CELITE::Deploy(void)
{
	m_flAccuracy = 0.88f;

	if (!(m_iClip & 1))
	{
		m_iWeaponState |= WPNSTATE_ELITE_LEFT;
	}

	return DefaultDeploy("models/v_elite.mdl", "models/p_elite.mdl", ELITE_DRAW, "dualpistols", UseDecrement() != FALSE);
}

void CELITE::PrimaryAttack(void)
{
	if (!(m_pPlayer->pev->flags & FL_ONGROUND))
	{
		ELITEFire(1.3 * (1 - m_flAccuracy), 0.2, FALSE);
	}
	else if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		ELITEFire(0.175 * (1 - m_flAccuracy), 0.2, FALSE);
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		ELITEFire(0.08 * (1 - m_flAccuracy), 0.2, FALSE);
	}
	else
	{
		ELITEFire(0.1 * (1 - m_flAccuracy), 0.2, FALSE);
	}
}

void CELITE::ELITEFire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	float flTimeDiff;
	int flag;
	Vector vecAiming;
	Vector vecSrc;
	Vector vecDir;

#ifdef REGAMEDLL_FIXES
	flCycleTime -= 0.078f;
#else
	flCycleTime -= 0.125f;
#endif

	if (++m_iShotsFired > 1)
	{
		return;
	}

	flTimeDiff = gpGlobals->time - m_flLastFire;

	if (m_flLastFire)
	{
		m_flAccuracy -= (0.325f - flTimeDiff) * 0.275f;

		if (m_flAccuracy > 0.88f)
		{
			m_flAccuracy = 0.88f;
		}
		else if (m_flAccuracy < 0.55f)
		{
			m_flAccuracy = 0.55f;
		}
	}

	m_flLastFire = gpGlobals->time;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

#ifndef CLIENT_DLL
		if (TheBots != NULL)
		{
			TheBots->OnEvent(EVENT_WEAPON_FIRED_ON_EMPTY, m_pPlayer);
		}
#endif

		return;
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	--m_iClip;
	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	vecSrc = m_pPlayer->GetGunPosition();
	vecAiming = gpGlobals->v_forward;

#ifdef CLIENT_WEAPONS
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	if (m_iWeaponState & WPNSTATE_ELITE_LEFT)
	{
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif
		m_iWeaponState &= ~WPNSTATE_ELITE_LEFT;

		vecDir = m_pPlayer->FireBullets3(vecSrc - gpGlobals->v_right * 5, vecAiming, flSpread,
			8192, BULLET_PLAYER_9MM, 1, ELITE_DAMAGE, ELITE_RANGE_MODIFER, m_pPlayer->pev, true, m_pPlayer->random_seed);

		PLAYBACK_EVENT_FULL(flag, m_pPlayer->edict(), m_usFireELITE_LEFT, 0, (float *)&g_vecZero, (float *)&g_vecZero, flTimeDiff, vecDir.x,
			int(vecDir.y * 100), m_iClip, FALSE, FALSE);
	}
	else
	{
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_ATTACK2);
#endif
		m_iWeaponState |= WPNSTATE_ELITE_LEFT;

		vecDir = m_pPlayer->FireBullets3(vecSrc + gpGlobals->v_right * 5, vecAiming, flSpread,
			8192, BULLET_PLAYER_9MM, 1, ELITE_DAMAGE, ELITE_RANGE_MODIFER, m_pPlayer->pev, true, m_pPlayer->random_seed);

		PLAYBACK_EVENT_FULL(flag, m_pPlayer->edict(), m_usFireELITE_RIGHT, 0, (float *)&g_vecZero, (float *)&g_vecZero, flTimeDiff, vecDir.x,
			int(vecDir.y * 100), m_iClip, FALSE, FALSE);
	}

#ifndef CLIENT_DLL
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
#endif

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	m_pPlayer->pev->punchangle.x -= 2.0f;
}

void CELITE::Reload(void)
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	if (DefaultReload(iMaxClip(), ELITE_RELOAD, ELITE_RELOAD_TIME))
	{
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD);
#endif
		m_flAccuracy = 0.88f;
	}
}

void CELITE::WeaponIdle(void)
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		if (m_iClip)
		{
			int iAnim = (m_iClip == 1) ? ELITE_IDLE_LEFTEMPTY : ELITE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0f;
			SendWeaponAnim(iAnim, UseDecrement() != FALSE);
		}
	}
}
