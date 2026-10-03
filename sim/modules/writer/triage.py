"""Assigns a Priority tag to each confirmed Event.

Zone: WRITER.
Inputs: writer/event -> Event.
Outputs: writer/triage -> TriageDecision.
Active in: always (no ACTIVE_IN set).
Params read from config/wiring.yaml: none.
P1 status: fixed severity*confidence bins (policy()) so the pipeline runs end-to-end;
never returns IGNORE.
P2 plan: replace policy() with a LinUCB bandit trained in sim (reward from ground
truth); INPUTS/OUTPUTS and TriageDecision contract are unchanged.
"""
from core.module import Module
from core.zones import Zone
from contracts.messages import TriageDecision, Priority

class Triage(Module):
    """P1: severity bins so the pipeline runs end-to-end.
    Replace policy() with LinUCB trained in sim (reward from ground truth) — interface unchanged."""
    ZONE = Zone.WRITER; INPUTS = ("writer/event",); OUTPUTS = ("writer/triage",)
    def policy(self, e):
        """Map an Event's severity*confidence score to a Priority bin."""
        s = e.severity * e.conf
        return Priority.IMMEDIATE if s > .8 else Priority.HIGH if s > .6 else Priority.NORMAL if s > .4 else Priority.LOW
    def step(self, t, inbox):
        """Triage every Event received this tick into a TriageDecision."""
        return {"writer/triage": [TriageDecision(self.hdr(t), e.id, self.policy(e)) for e in inbox["writer/event"]]}
