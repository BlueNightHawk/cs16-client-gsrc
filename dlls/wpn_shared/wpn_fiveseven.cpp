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

LINK_ENTITY_TO_CLASS(weapon_fiveseven, CFiveSeven)

void CFiveSeven::Spawn(void)
{
	pev->classname = MAKE_STRING("weapon_fiveseven");

	Precache();

	m_iId = WEAPON_FIVESEVEN;
	SET_MODEL(edict(), "models/w_fiveseven.mdl");

	m_iDefaultAmmo = FIVESEVEN_DEFAULT_GIVE;
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
	m_flAccuracy = 0.92f;

	FallInit();
}

void CFiveSeven::Precache(void)
{
	PRECACHE_MODEL("models/v_fiveseven.mdl");
	PRECACHE_MODEL("models/w_fiveseven.mdl");
	PRECACHE_MODEL("models/shield/v_shield_fiveseven.mdl");

	PRECACHE_SOUND("weapons/fiveseven-1.wav");
	PRECACHE_SOUND("weapons/fiveseven_clipout.wav");
	PRECACHE_SOUND("weapons/fiveseven_clipin.wav");
	PRECACHE_SOUND("weapons/fiveseven_sliderelease.wav");
	PRECACHE_SOUND("weapons/fiveseven_slidepull.wav");

	m_iShell = PRECACHE_MODEL("models/pshell.mdl");
	m_usFireFiveSeven = PRECACHE_EVENT(1, "events/fiveseven.sc");
}

int CFiveSeven::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "57mm";
	p->iMaxAmmo1 = MAX_AMMO_57MM;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = FIVESEVEN_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 6;
	p->iId = m_iId = WEAPON_FIVESEVEN;
	p->iFlags = 0;
	p->iWeight = FIVESEVEN_WEIGHT;

	return 1;
}

BOOL CFiveSeven::Deploy(void)
{
	m_flAccuracy = 0.92f;
	m_fMaxSpeed = FIVESEVEN_MAX_SPEED;
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
	m_pPlayer->m_bShieldDrawn = false;

	if (m_pPlayer->HasShield())
		return DefaultDeploy("models/shield/v_shield_fiveseven.mdl", "models/shield/p_shield_fiveseven.mdl", FIVESEVEN_DRAW, "shieldgun", UseDecrement() != FALSE);
	else
		return DefaultDeploy("models/v_fiveseven.mdl", "models/p_fiveseven.mdl", FIVESEVEN_DRAW, "onehanded", UseDecrement() != FALSE);
}

void CFiveSeven::PrimaryAttack(void)
{
	if (!(m_pPlayer->pev->flags & FL_ONGROUND))
	{
		FiveSevenFire(1.5 * (1 - m_flAccuracy), 0.2, FALSE);
	}
	else if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		FiveSevenFire(0.255 * (1 - m_flAccuracy), 0.2, FALSE);
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		FiveSevenFire(0.075 * (1 - m_flAccuracy), 0.2, FALSE);
	}
	else
	{
		FiveSevenFire(0.15 * (1 - m_flAccuracy), 0.2, FALSE);
	}
}

void CFiveSeven::SecondaryAttack(void)
{
	ShieldSecondaryFire(SHIELDGUN_UP, SHIELDGUN_DOWN);
}

void CFiveSeven::FiveSevenFire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	Vector vecAiming, vecSrc, vecDir;
	int flag;

	flCycleTime -= 0.05;

	if (++m_iShotsFired > 1)
	{
		return;
	}

	if (m_flLastFire != 0.0f)
	{
		m_flAccuracy -= (0.275f - (gpGlobals->time - m_flLastFire)) * 0.25f;

		if (m_flAccuracy > 0.92f)
		{
			m_flAccuracy = 0.92f;
		}
		else if (m_flAccuracy < 0.725f)
		{
			m_flAccuracy = 0.725f;
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

	--m_iClip;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	SetPlayerShieldAnim();

#ifndef CLIENT_DLL
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	vecSrc = m_pPlayer->GetGunPosition();
	vecAiming = gpGlobals->v_forward;

	vecDir = m_pPlayer->FireBullets3(vecSrc, vecAiming, flSpread, 4096, 1, BULLET_PLAYER_57MM, FIVESEVEN_DAMAGE, FIVESEVEN_RANGE_MODIFER, m_pPlayer->pev, false, m_pPlayer->random_seed);

#ifdef CLIENT_WEAPONS
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	PLAYBACK_EVENT_FULL(flag, m_pPlayer->edict(), m_usFireFiveSeven, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		int(m_pPlayer->pev->punchangle.x * 100), int(m_pPlayer->pev->punchangle.y * 100), m_iClip == 0, FALSE);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

#ifndef CLIENT_DLL
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, FALSE);
	}
#endif

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	m_pPlayer->pev->punchangle.x -= 2.0f;
	ResetPlayerShieldAnim();
}

void CFiveSeven::Reload(void)
{
	if (m_pPlayer->ammo_57mm <= 0)
		return;

	if (DefaultReload(iMaxClip(), FIVESEVEN_RELOAD, FIVESEVEN_RELOAD_TIME))
	{
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD);
#endif
		m_flAccuracy = 0.92f;
	}
}

void CFiveSeven::WeaponIdle(void)
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
	{
		return;
	}

	if (m_pPlayer->HasShield())
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20.0f;

		if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
		{
			SendWeaponAnim(SHIELDGUN_DRAWN_IDLE, UseDecrement() != FALSE);
		}
	}
	else if (m_iClip)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0625f;
		SendWeaponAnim(FIVESEVEN_IDLE, UseDecrement() != FALSE);
	}
}
