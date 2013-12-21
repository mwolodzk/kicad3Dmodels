/*
 file: gendiode.cpp

 Copyright 2012, Cirilo Bernardo (cjh.bernardo@gmail.com)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>

 */

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "vdefs.h"
#include "vcom.h"
#include "gendiode.h"
#include "transform.h"
#include "vrmlmat.h"
#include "circle.h"
#include "pin.h"

using namespace std;
using namespace kc3d;

namespace kc3ddiode
{

GenDiode::GenDiode()
{
    hascol = false;
    nwv = 16;
    ntv = 48;
    hasparams = false;
    dwire = 0.0;
    dbody = 0.0;
    lbody = 0.0;
    wcath = 0.0;
    scath = 0.0;

    return;
}

GenDiode::~GenDiode()
{
    return;
}

int GenDiode::SetParams(double wire, double body, double length, double band, double space)
{
    hasparams = false;
    if (wire < 1e-9)
    {
        ERRBLURB;
        cerr << "invalid wire diameter; must be > 0\n";
        return -1;
    }
    if (body <= wire)
    {
        ERRBLURB;
        cerr << "invalid body diameter; must be >= wire diameter\n";
        return -1;
    }
    if (length <= wire)
    {
        ERRBLURB;
        cerr << "invalid body length; must be >= wire diameter\n";
        return -1;
    }
    if (space < 0)
    {
        ERRBLURB;
        cerr << "invalid space between end and cathode band; must be >= 0\n";
        return -1;
    }
    if (band < length*0.01)
    {
        ERRBLURB;
        cerr << "invalid cathode band width; must be >= length*0.01\n";
        return -1;
    }
    if (band >= length*0.3)
    {
        ERRBLURB;
        cerr << "invalid cathode band width; must be < length*0.3\n";
        return -1;
    }
    if ( (band + space) > (length*0.4) )
    {
        ERRBLURB;
        cerr << "invalid parameters; band + space must be < length*0.4 to minimize ambiguity\n";
        return -1;
    }
    hasparams = true;
    dwire = wire;
    dbody = body;
    lbody = length;
    wcath = band;
    scath = space;
    return 0;
}

int GenDiode::SetNVertices(int wire, int tube, int bend)
{
    if ((wire < 3) || (wire > 360))
    {
        ERRBLURB;
        cerr << "wire must have from 3 to 360 vertices\n";
        return -1;
    }
    if ((tube < 3) || (tube > 360))
    {
        ERRBLURB;
        cerr << "tube must have from 3 to 360 vertices\n";
        return -1;
    }
    if ((bend < 2) || (bend > 90))
    {
        ERRBLURB;
        cerr << "bend must have a value from 2 to 90\n";
        return -1;
    }
    nwv = wire;
    ntv = tube;
    nb = bend;
    return 0;
}

int GenDiode::SetColors(const std::string &wirecolor, const std::string &bodycolor,
        const std::string &cathodecolor)
{
    int acc = 0;
    acc += wcol.Load(wirecolor);
    acc += bcol.Load(bodycolor);
    acc += kcol.Load(cathodecolor);
    if (acc)
    {
        hascol = false;
        ERRBLURB;
        cerr << "problems loading colors\n";
        return -1;
    }
    hascol = true;
    return 0;
}

int GenDiode::Build(const std::string &partname, double scale, bool horiz, bool vflip, double pitch, double lead)
{
    /*
     * Dimensions per DO-35 package specification:
     *      + D: 1.53 .. 2.28   (body diameter)
     *      + B: 3.05 .. 4.15   (body length)
     *      + C: 0.46 .. 0.56   (lead diameter)
     *      + A: 25.40 .. 38.10 (lead length - doesn't really matter)
     *
     * Other dimensions:
     *      + 0.9: edge taper (chamfer is 10% of body diameter)
     *      + 0.5: thickness of cathode band
     *      + 0.7: inner tube is 70% of outer body size
     *      + 0.3: 0.3mm is the thickness of the glass at the ends
     *      + vertical offsets:
     *          > horiz: ~D/2 + C == 1.7 mm to axis of body
     *          > vert: C == 0.56 to edge of body
     */

    Circle body[6]; // outer body (including cathode)
    Pin leads[2];

    // constraint: colors must be successfully loaded (hascol = true)
    if (!hascol)
    {
        ERRBLURB;
        cerr << "invoked without prior successful call to SetColors()\n";
        return -1;
    }

    // constraint: parameters must have been set
    if (!hasparams)
    {
        ERRBLURB;
        cerr << "invoked without prior successful call to SetParams()\n";
        return -1;
    }

    // constraint: pitch must be:
    //      horiz: >= length + 3*wire diameter (to allow for bends)
    //      vert: >= body diameter + 0.5*wire diameter
    if ((horiz) && (pitch < (lbody + 3.0*dwire)))
    {
        ERRBLURB;
        cerr << "pitch (" << pitch << ") is below minimum for horizontal orientation (body length + 3*wire dia.)\n";
        return -1;
    }
    else if ((!horiz) && (pitch < (0.5*(dbody + dwire))))
    {
        ERRBLURB;
        cerr << "pitch (" << pitch << ") is below minimum for vertical orientation (0.5*(body dia. + wire dia.))\n";
        return -1;
    }

    // constraint: vflip may only be true for vert orientation (otherwise ignored, but a warning is issued)
    if (horiz && vflip)
    {
        ERRBLURB;
        cerr << "WARNING: vflip is true while orientation is horizontal\n";
        vflip = false;
    }

    // constraint: lead must be >= 2.0
    if (lead < 2.0)
    {
        ERRBLURB;
        cerr << "lead length below top of PCB (" << lead << "must be >= 2.0 mm\n";
        return -1;
    }

    int i, j;
    for (i = 0; i < 6; ++i) body[i].SetNVertices(ntv);

    int nc; // number of circles used in description (only 5 if 'space' is zero, otherwise 6)
    if (scath < 1e-9)
        nc = 5;
    else
        nc = 6;

    Transform t0, t1;
    Quat q0;
    double r, l0, l1, l2;   // bend radius and lengths/offsets
    PParams pp;
    pp.tap = -1.0;
    pp.w = dwire;
    pp.d = dwire;
    leads[0].SetShape(false);
    leads[1].SetShape(false);
    if (horiz)
    {
        // leads
        t0.setTranslation(0, 0, -lead);
        r = 1.5*dwire;
        l0 = dbody/2.0 + dwire + lead - r;  // length of vertical section
        l1 = (pitch - lbody)/2.0 - r;       // length of horizontal section
        pp.h = l0;
        pp.r = r;
        pp.nb = nb;
        pp.l = l1;
        pp.bend = M_PI/2.0;
        pp.ns = nwv;
        leads[0].Calc(pp, t0);
        t0.setTranslation(pitch, 0, -lead);
        t0.setRotation(M_PI, 0, 0, 1);
        leads[1].Calc(pp, t0);
        // body
        j = 0;
        l2 = 0.5*dbody + dwire;
        l0 = (pitch - lbody)/2.0;
        t0.setTranslation(l0, 0, l2);
        t0.setRotation(M_PI/2.0, 0, 1, 0);
        l1 = 0.98*dbody;
        body[j++].Calc(l1, l1, t0);
        l0 += 0.02*dbody;
        t0.setTranslation(l0, 0, l2);
        body[j++].Calc(dbody, dbody, t0);
        if (scath > 1e-9)
        {
            l1 = scath - 0.02*dbody;
            if (l1 > 0)
            {
                l0 += l1;
                t0.setTranslation(l0, 0, l2);
                body[j++].Calc(dbody, dbody, t0);
            }
            else
            {
                nc = 5;
            }
        }
        l0 += wcath;
        t0.setTranslation(l0, 0, l2);
        body[j++].Calc(dbody, dbody, t0);
        l0 = (pitch - lbody)/2.0 + 0.98*lbody;
        t0.setTranslation(l0, 0, l2);
        body[j++].Calc(dbody, dbody, t0);
        l0 += 0.02*lbody;
        t0.setTranslation(l0, 0, l2);
        l1 = 0.98*dbody;
        body[j++].Calc(l1, l1, t0);
    }
    else
    {
        // leads
        t0.setTranslation(0, 0, -lead);
        l0 = lead + dwire;
        pp.h = l0;
        pp.r = -1;
        pp.nb = nb;
        pp.l = -1;
        pp.bend = 0.0;
        pp.ns = nwv;
        leads[0].Calc(pp, t0);
        t0.setTranslation(0, 0, dwire + lbody);
        pp.h = dwire;
        pp.r = pitch/2.0;
        pp.l = lead + 2.0*dwire + lbody;
        pp.bend = M_PI;
        leads[1].Calc(pp, t0);
        // body
        j = 0;
        t0.setTranslation(0, 0, dwire);
        l1 = 0.98*dbody;
        body[j++].Calc(l1, l1, t0);
        l0 = dwire + 0.02*dbody;
        t0.setTranslation(0, 0, l0);
        body[j++].Calc(dbody, dbody, t0);
        if (!vflip)
        {
            // cathode is at short lead
            if (scath > 1e-9)
            {
                l1 = scath - 0.02*dbody;
                if (l1 > 0)
                {
                    l0 += l1;
                    t0.setTranslation(0, 0, l0);
                    body[j++].Calc(dbody, dbody, t0);
                }
                else
                {
                    nc = 5;
                }
            }
            l0 += wcath;
            t0.setTranslation(0, 0, l0);
            body[j++].Calc(dbody, dbody, t0);
        }
        else
        {
            // cathode is at long lead
            l0 = dwire + lbody - scath - wcath;
            t0.setTranslation(0, 0, l0);
            body[j++].Calc(dbody, dbody, t0);
            if (scath > 1e-9)
            {
                l1 = scath - 0.02*dbody;
                if (l1 > 0)
                {
                    l0 += wcath;
                    t0.setTranslation(0, 0, l0);
                    body[j++].Calc(dbody, dbody, t0);
                }
                else
                {
                    nc = 5;
                }
            }
        }
        l0 = dwire + lbody - 0.02*dbody;
        t0.setTranslation(0, 0, l0);
        body[j++].Calc(dbody, dbody, t0);
        l1 = dbody*0.98;
        l0 = dwire + lbody;
        t0.setTranslation(0, 0, l0);
        body[j++].Calc(l1, l1, t0);
    }

    int acc = 0;

    // open the file
    ofstream fp;
    ostringstream filename;
    filename << partname << ".wrl";
    acc += SetupVRML(filename.str(), fp);
    if (acc)
    {
        ERRBLURB;
        cerr << "could not open output file '" << filename.str().c_str() << "'\n";
        return -1;
    }
    acc += SetupXForm(partname, fp, 0);

    l0 = pitch/2.0;
    t0.setTranslation(-l0, 0, 0);
    t0.setRotation(0, 0, 0, 0);
    t0.setScale(scale);
    if (horiz)
    {
        // leads
        acc += leads[0].Build(true, false, t0, wcol, false, fp, 2);
        acc += leads[1].Build(true, false, t0, wcol, true, fp, 2);
        // body
        acc += body[0].Paint(false, t0, bcol, false, fp, 2);
        acc += body[nc -1].Paint(true, t0, bcol, true, fp, 2);
        acc += body[0].Stitch(true, body[1], t0, bcol, true, fp, 2);
        j = 1;
        if (nc == 6)
        {
            acc += body[j].Stitch(true, body[j+1], t0, bcol, true, fp, 2);
            ++j;
        }
        acc += body[j].Stitch(true, body[j+1], t0, kcol, false, fp, 2);
        while (++j < (nc -1))
        {
            acc += body[j].Stitch(true, body[j+1], t0, bcol, true, fp, 2);
        }
    }
    else
    {
        if( vflip )
        {
            t0.setRotation(M_PI, 0, 0, 1);
            t0.setTranslation(l0, 0, 0);
        }

        // leads
        acc += leads[0].Build(true, false, t0, wcol, false, fp, 2);
        acc += leads[1].Build(false, true, t0, wcol, true, fp, 2);
        // body
        if (!vflip)
        {
            acc += body[0].Paint(false, t0, bcol, false, fp, 2);
            acc += body[nc-1].Paint(true, t0, bcol, true, fp, 2);
            acc += body[0].Stitch(true, body[1], t0, bcol, true, fp, 2);
            j = 1;
            if (nc == 6)
            {
                acc += body[j].Stitch(true, body[j+1], t0, bcol, true, fp, 2);
                ++j;
            }
            acc += body[j].Stitch(true, body[j+1], t0, kcol, false, fp, 2);
            while (++j < (nc -1))
            {
                acc += body[j].Stitch(true, body[j+1], t0, bcol, true, fp, 2);
            }
        }
        else
        {
            acc += body[0].Paint(false, t0, bcol, false, fp, 2);
            acc += body[nc-1].Paint(true, t0, bcol, true, fp, 2);
            acc += body[0].Stitch(true, body[1], t0, bcol, true, fp, 2);
            acc += body[1].Stitch(true, body[2], t0, bcol, true, fp, 2);
            acc += body[2].Stitch(true, body[3], t0, kcol, false, fp, 2);
            j = 3;
            if (nc == 6)
            {
                acc += body[j].Stitch(true, body[j+1], t0, bcol, true, fp, 2);
                ++j;
            }
            acc += body[j].Stitch(true, body[j+1], t0, bcol, true, fp, 2);
        }
    }

    acc += CloseXForm(fp, 0);
    fp.close();

    if (acc)
    {
        ERRBLURB;
        cerr << "problems writing diode data to file\n";
        return -1;
    }

    return 0;
}   // Build()

} /* namespace kc3d */
